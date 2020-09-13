--
-- PostgreSQL database dump
--

-- Dumped from database version 10.6
-- Dumped by pg_dump version 10.6

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET client_min_messages = warning;
SET row_security = off;

--
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


--
-- Name: case_modification_constraints(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.case_modification_constraints() RETURNS trigger
    LANGUAGE plpgsql
    AS $$DECLARE
  intVar integer;
BEGIN
  IF TG_OP = 'INSERT' THEN
    return new;
  END IF;

  IF TG_OP = 'DELETE' THEN
    IF old.status > 0 THEN
      RAISE EXCEPTION 'Нельзя удалить утвержденный кейс';
    END IF;
    return old;
  END IF;

  IF TG_OP = 'UPDATE' THEN
    IF old.status = new.status AND old.status > 0 AND (old.id<>new.id OR old.title<>new.title OR old.icon<>new.icon OR old.description<>new.description 
      OR old.passing_score<>new.passing_score OR old.min_age<>new.min_age OR old.author_id<>new.author_id OR old.max_score<>new.max_score 
      OR old.task_count<>new.task_count) THEN
      RAISE EXCEPTION 'Нельзя изменять утвержденный или деактивированный кейс';
    END IF;

    IF old.status=1 AND new.status=2 THEN
      SELECT count('x') FROM learning_group_case 
      WHERE case_id=new.id AND state=1 INTO intVar;
      IF intVar > 0 THEN
        RAISE EXCEPTION 'Деактивация запрещена. Кейс используется в % групп(е/ах)', intVar;
      END IF;
    END IF;
    
    return new;
  END IF;
END$$;


ALTER FUNCTION public.case_modification_constraints() OWNER TO postgres;

--
-- Name: case_passed(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.case_passed() RETURNS trigger
    LANGUAGE plpgsql
    AS $$DECLARE
  passed boolean;
  caseId integer;
  userId integer;
BEGIN
  caseId := 0;
  userId := 0;

  IF (TG_OP = 'DELETE') THEN
    IF (old.passed) THEN
      passed := false;
	  caseId := old.case_id;
	  userId := old.user_id;
    ELSE
	  RETURN new;
	END IF;
  END IF;
  
  IF (TG_OP = 'INSERT') THEN
    IF (new.passed) THEN
      passed := true;
	  caseId := new.case_id;
	  userId := new.user_id;
    ELSE
  	  RETURN old;
    END IF;
  END IF;	
  
  IF (TG_OP = 'UPDATE') THEN
    IF (old.passed <> new.passed) THEN
      passed := new.passed;
	  caseId := new.case_id;
	  userId := new.user_id;
    ELSE
      RETURN new;
    END IF;
  END IF;	
  
  IF (passed) THEN
	  INSERT INTO user_achievement(user_id, achievement_id)
		SELECT userId, ca.achievement_id FROM case_achievement ca
		LEFT JOIN user_achievement ua ON ca.achievement_id=ua.achievement_id and ua.user_id=userId
		  WHERE case_id=caseId and ua.id is NULL;  
  ELSE 
		DELETE FROM user_achievement WHERE user_id=userId
		  AND achievement_id NOT IN 
		    (SELECT ca.achievement_id FROM user_case uc 
		     LEFT JOIN case_achievement ca on uc.case_id=ca.case_id
		     WHERE uc.passed and uc.user_id=userId and uc.case_id<>caseId)
		  AND achievement_id NOT IN
		     (SELECT cra.achievement_id FROM case_required_achievement cra
		      INNER JOIN user_case uc on cra.case_id=uc.case_id
		      WHERE uc.passed and uc.user_id=userId and uc.case_id<>caseId);
  END IF;
  
  IF (TG_OP = 'DELETE') THEN
    RETURN old;
  ELSE 
    RETURN new;
  END IF;
END$$;


ALTER FUNCTION public.case_passed() OWNER TO postgres;

--
-- Name: FUNCTION case_passed(); Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON FUNCTION public.case_passed() IS 'При прохождении кейса добавляет пользователю достижения и наоборот';


--
-- Name: check_achievement_loops(integer); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.check_achievement_loops(achievement_node_id integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$DECLARE
  has_intersections boolean;
BEGIN
SELECT EXISTS((WITH RECURSIVE temp1 (id, achievement_id, child_id) AS
(SELECT t1.id, t1.achievement_id, t1.child_id FROM achievement_children t1 
WHERE t1.achievement_id=achievement_node_id
union
select t2.id, t2.achievement_id, t2.child_id FROM achievement_children t2 INNER JOIN temp1 on temp1.child_id=t2.achievement_id)
select temp1.child_id from temp1
where temp1.child_id is not null)
INTERSECT
(WITH RECURSIVE temp1 (id, achievement_id, child_id) AS
(SELECT t1.id, t1.achievement_id, t1.child_id FROM achievement_children t1 
WHERE t1.child_id=achievement_node_id
union
select t2.id, t2.achievement_id, t2.child_id FROM achievement_children t2 INNER JOIN temp1 on temp1.achievement_id=t2.child_id)
select temp1.achievement_id from temp1
)) INTO has_intersections;

RETURN has_intersections;
END$$;


ALTER FUNCTION public.check_achievement_loops(achievement_node_id integer) OWNER TO postgres;

--
-- Name: child_achievement_check(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.child_achievement_check() RETURNS trigger
    LANGUAGE plpgsql
    AS $$DECLARE
  loopFound boolean;
BEGIN
  select check_achievement_loops(new.child_id) into loopFound;
  
  if (loopFound) then
    raise 'ErrAchievementLoops: Выбранная зависимость приводит к логической ошибке иерархии достижений.';
  end if;
  return new;
END$$;


ALTER FUNCTION public.child_achievement_check() OWNER TO postgres;

--
-- Name: clone_case(bigint, bigint); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.clone_case(source_case_id bigint, user_id bigint) RETURNS bigint
    LANGUAGE plpgsql
    AS $$DECLARE
  new_case_id bigint;
BEGIN
  SELECT id FROM "case" WHERE id = source_case_id INTO new_case_id;

  IF new_case_id IS NULL THEN
    RAISE EXCEPTION 'Попытка клонирования несуществующего кейса (id=%)', source_case_id;
  END IF;

  INSERT INTO "case"(title, icon, description, passing_score, min_age, author_id)
    SELECT title, icon, description, passing_score, min_age, user_id FROM "case" 
      WHERE id = source_case_id RETURNING id INTO new_case_id;

  INSERT INTO case_required_achievement(case_id, achievement_id)
    SELECT new_case_id, achievement_id FROM case_required_achievement 
      WHERE case_id = source_case_id;   

  INSERT INTO case_achievement(case_id, achievement_id)
    SELECT new_case_id, achievement_id FROM case_achievement 
      WHERE case_id = source_case_id;

  INSERT INTO case_unit(title, sort_id, description, case_id)
    SELECT title, sort_id, description, new_case_id FROM case_unit
      WHERE case_id = source_case_id ORDER BY sort_id;

  INSERT INTO task(name, max_score, sort_id, description, case_id)
    SELECT t.name, t.max_score, t.sort_id, t.description, new_case_id
    FROM task t
    WHERE t.case_id = source_case_id
    ORDER BY t.sort_id;

  RETURN new_case_id;
END$$;


ALTER FUNCTION public.clone_case(source_case_id bigint, user_id bigint) OWNER TO postgres;

--
-- Name: delete_achievement_dependants(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.delete_achievement_dependants() RETURNS trigger
    LANGUAGE plpgsql
    AS $$BEGIN
  DELETE FROM user_achievement
  WHERE user_id=old.user_id
    AND achievement_id IN
      (SELECT achievement_id FROM achievement_children
       WHERE child_id=old.achievement_id);

  RETURN old;
END$$;


ALTER FUNCTION public.delete_achievement_dependants() OWNER TO postgres;

--
-- Name: FUNCTION delete_achievement_dependants(); Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON FUNCTION public.delete_achievement_dependants() IS 'При удалении достижения у пользователя удаляет все зависимые.';


--
-- Name: delete_required_case_achievements(integer, integer); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.delete_required_case_achievements(caseid integer, achievementid integer) RETURNS void
    LANGUAGE plpgsql
    AS $$BEGIN
  DELETE FROM case_required_achievement WHERE case_id=caseid AND achievement_id in
    (SELECT ac.child_id
     FROM achievement_children ac where ac.achievement_id=achievementid
     EXCEPT
     SELECT DISTINCT ac2.child_id
     FROM achievement_children ac2 where 
       achievement_id in (select achievement_id from case_achievement where case_id=caseid));
  RETURN;     
END$$;


ALTER FUNCTION public.delete_required_case_achievements(caseid integer, achievementid integer) OWNER TO postgres;

--
-- Name: insert_city_default_timing_grid(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.insert_city_default_timing_grid() RETURNS trigger
    LANGUAGE plpgsql
    AS $$BEGIN
  INSERT INTO timing_grid (name, city_id, duration, city_default)
    VALUES (new.name || '(по умолчанию)', new.id, 40, true);
  RETURN new;
END$$;


ALTER FUNCTION public.insert_city_default_timing_grid() OWNER TO postgres;

--
-- Name: on_insert_case_achievements(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.on_insert_case_achievements() RETURNS trigger
    LANGUAGE plpgsql
    AS $$DECLARE
  already_in_case_required_achievements boolean;
BEGIN
  select exists(select 'x' from case_required_achievement where case_id=new.case_id and achievement_id=new.achievement_id)
  into already_in_case_required_achievements;

  IF already_in_case_required_achievements THEN
    RAISE 'ErrAlreadyInCaseRequiredAchievements';
  END IF;

  INSERT INTO case_required_achievement (case_id, achievement_id)
    SELECT new.case_id, child_id
    FROM achievement_children ac WHERE ac.achievement_id=new.achievement_id
    EXCEPT
    SELECT new.case_id, cra.achievement_id
    FROM case_required_achievement cra WHERE cra.case_id=new.case_id;
  RETURN new;
END$$;


ALTER FUNCTION public.on_insert_case_achievements() OWNER TO postgres;

--
-- Name: on_insert_case_required_achievements(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.on_insert_case_required_achievements() RETURNS trigger
    LANGUAGE plpgsql
    AS $$DECLARE
  already_in_case_achievements boolean;
BEGIN
  select exists(select 'x' from case_achievement where case_id=new.case_id and achievement_id=new.achievement_id)
  into already_in_case_achievements;

  IF already_in_case_achievements THEN
    RAISE 'ErrAlreadyInCaseAchievements';
  END IF;

  RETURN new;
END$$;


ALTER FUNCTION public.on_insert_case_required_achievements() OWNER TO postgres;

--
-- Name: update_case_after_case_unit(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.update_case_after_case_unit() RETURNS trigger
    LANGUAGE plpgsql
    AS $$DECLARE
  recCaseId bigint;
BEGIN
  IF TG_OP = 'DELETE' THEN
    recCaseId := old.case_id;
  ELSE
    recCaseId := new.case_id;
  END IF;

  UPDATE "case" 
  SET (unit_count)=(SELECT count('x') FROM case_unit WHERE case_id=recCaseId)
  WHERE id=recCaseId;
  
  IF TG_OP = 'DELETE' THEN
    RETURN old;
  ELSE
    RETURN new;
  END IF;
END$$;


ALTER FUNCTION public.update_case_after_case_unit() OWNER TO postgres;

--
-- Name: FUNCTION update_case_after_case_unit(); Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON FUNCTION public.update_case_after_case_unit() IS 'Обновление полей кейса при изменении уроков';


--
-- Name: update_case_unit_after_task(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.update_case_unit_after_task() RETURNS trigger
    LANGUAGE plpgsql
    AS $$DECLARE
  recCaseId bigint;
BEGIN
  IF TG_OP = 'DELETE' THEN
    recCaseId := old.case_id;
  ELSE
    recCaseId := new.case_id;
  END IF;

  UPDATE "case"
  SET (max_score, task_count)=(SELECT coalesce(sum(max_score),0), count('x') FROM task WHERE case_id=recCaseId)
  WHERE id=recCaseId;
  
  IF TG_OP = 'DELETE' THEN
    RETURN old;
  ELSE
    RETURN new;
  END IF;
END$$;


ALTER FUNCTION public.update_case_unit_after_task() OWNER TO postgres;

--
-- Name: FUNCTION update_case_unit_after_task(); Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON FUNCTION public.update_case_unit_after_task() IS 'Обновление полей урока при изменении заданий';


--
-- Name: user_achievement_delete(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.user_achievement_delete() RETURNS trigger
    LANGUAGE plpgsql
    AS $$BEGIN
  update user_achievement ua
  set expire_date = 'today'::date + 30*(select valid_months from achievement where id=ua.achievement_id)
  where ua.user_id=old.user_id
  and ua.achievement_id in
  (select a.id from achievement_children ac
   inner join achievement a on ac.child_id=a.id and a.valid_months>0
   left join achievement_children ac2 on ac.child_id=ac2.child_id and ac2.achievement_id<>old.achievement_id
   left join user_achievement ua2 on ac2.achievement_id=ua2.achievement_id
   where ac.achievement_id=old.achievement_id and ua2.id is null);

  RETURN old;
END$$;


ALTER FUNCTION public.user_achievement_delete() OWNER TO postgres;

--
-- Name: FUNCTION user_achievement_delete(); Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON FUNCTION public.user_achievement_delete() IS 'Начинает отсчет жизни предыдущих достижения пользователя';


--
-- Name: user_achievement_insert(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.user_achievement_insert() RETURNS trigger
    LANGUAGE plpgsql
    AS $$BEGIN
  update user_achievement ua
  set expire_date = NULL
  where ua.user_id=new.user_id
  and ua.achievement_id in
  (select a.id from achievement_children ac
  inner join achievement a on ac.child_id=a.id
  where ac.achievement_id=new.achievement_id);

  RETURN new;
END$$;


ALTER FUNCTION public.user_achievement_insert() OWNER TO postgres;

--
-- Name: FUNCTION user_achievement_insert(); Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON FUNCTION public.user_achievement_insert() IS 'Сбрасывает отсчет жизни дочерних достижений при добавлении';


--
-- Name: user_achievement_insert_expire(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.user_achievement_insert_expire() RETURNS trigger
    LANGUAGE plpgsql
    AS $$DECLARE
  months integer;
BEGIN
  select valid_months from achievement where id=new.achievement_id into months;

  IF months > 0 THEN
    new.expire_date = 'today'::date + 30*months;
  END IF;

  RETURN new;
END$$;


ALTER FUNCTION public.user_achievement_insert_expire() OWNER TO postgres;

--
-- Name: FUNCTION user_achievement_insert_expire(); Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON FUNCTION public.user_achievement_insert_expire() IS 'Добавляет дату протухания достижения';


--
-- Name: user_achievement_set_children(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION public.user_achievement_set_children() RETURNS trigger
    LANGUAGE plpgsql
    AS $$BEGIN
  INSERT INTO user_achievement (user_id, achievement_id)
    SELECT new.user_id, ac.child_id
    FROM achievement_children ac
    LEFT JOIN user_achievement ua ON ac.child_id=ua.achievement_id AND user_id=new.user_id
    WHERE ac.achievement_id=new.achievement_id AND ua.id IS NULL;
  
  RETURN new;
END$$;


ALTER FUNCTION public.user_achievement_set_children() OWNER TO postgres;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: achievement; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.achievement (
    id integer NOT NULL,
    title character varying(1000) NOT NULL,
    achievement_group_id integer NOT NULL,
    description character varying(500),
    valid_months smallint DEFAULT 0 NOT NULL
);


ALTER TABLE public.achievement OWNER TO postgres;

--
-- Name: achievement_children; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.achievement_children (
    id integer NOT NULL,
    achievement_id integer NOT NULL,
    child_id integer NOT NULL
);


ALTER TABLE public.achievement_children OWNER TO postgres;

--
-- Name: TABLE achievement_children; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.achievement_children IS 'Иерархия достижений';


--
-- Name: achievement_group; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.achievement_group (
    id integer NOT NULL,
    title character varying(100) NOT NULL,
    parent_id integer,
    city_id integer NOT NULL
);


ALTER TABLE public.achievement_group OWNER TO postgres;

--
-- Name: TABLE achievement_group; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.achievement_group IS 'Группа достижений';


--
-- Name: achievement_group_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.achievement_group_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.achievement_group_id_seq OWNER TO postgres;

--
-- Name: achievement_group_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.achievement_group_id_seq OWNED BY public.achievement_group.id;


--
-- Name: achievement_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.achievement_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.achievement_id_seq OWNER TO postgres;

--
-- Name: achievement_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.achievement_id_seq OWNED BY public.achievement.id;


--
-- Name: action_log; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.action_log (
    id bigint NOT NULL,
    dt timestamp without time zone DEFAULT now() NOT NULL,
    request_uid uuid,
    user_id bigint,
    action character varying(128) NOT NULL,
    success boolean NOT NULL,
    description character varying(4096)
);


ALTER TABLE public.action_log OWNER TO postgres;

--
-- Name: action_log_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.action_log_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.action_log_id_seq OWNER TO postgres;

--
-- Name: action_log_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.action_log_id_seq OWNED BY public.action_log.id;


--
-- Name: case; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public."case" (
    id integer NOT NULL,
    title character varying(200) NOT NULL,
    icon character varying(100) NOT NULL,
    description character varying(1000) NOT NULL,
    passing_score integer DEFAULT 50 NOT NULL,
    min_age smallint DEFAULT 6 NOT NULL,
    author_id integer NOT NULL,
    max_score integer DEFAULT 0 NOT NULL,
    unit_count smallint DEFAULT 0 NOT NULL,
    status smallint DEFAULT 0 NOT NULL,
    task_count smallint DEFAULT 0 NOT NULL,
    CONSTRAINT case_status_chk CHECK (((status >= 0) AND (status <= 2)))
);


ALTER TABLE public."case" OWNER TO postgres;

--
-- Name: TABLE "case"; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public."case" IS 'Кейсы';


--
-- Name: COLUMN "case".status; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public."case".status IS 'Статус кейса';


--
-- Name: case_achievement; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.case_achievement (
    id integer NOT NULL,
    case_id integer NOT NULL,
    achievement_id integer NOT NULL
);


ALTER TABLE public.case_achievement OWNER TO postgres;

--
-- Name: TABLE case_achievement; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.case_achievement IS 'Достижения получаемые при прохождении кейса';


--
-- Name: case_achievement_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.case_achievement_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.case_achievement_id_seq OWNER TO postgres;

--
-- Name: case_achievement_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.case_achievement_id_seq OWNED BY public.case_achievement.id;


--
-- Name: case_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.case_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.case_id_seq OWNER TO postgres;

--
-- Name: case_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.case_id_seq OWNED BY public."case".id;


--
-- Name: case_required_achievement; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.case_required_achievement (
    id integer NOT NULL,
    case_id integer NOT NULL,
    achievement_id integer NOT NULL
);


ALTER TABLE public.case_required_achievement OWNER TO postgres;

--
-- Name: TABLE case_required_achievement; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.case_required_achievement IS 'Необходимые для записи на кейс достижения';


--
-- Name: case_required_achievement_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.case_required_achievement_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.case_required_achievement_id_seq OWNER TO postgres;

--
-- Name: case_required_achievement_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.case_required_achievement_id_seq OWNED BY public.case_required_achievement.id;


--
-- Name: case_unit; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.case_unit (
    id bigint NOT NULL,
    title character varying(200) NOT NULL,
    sort_id smallint NOT NULL,
    description character varying(1024),
    case_id integer
);


ALTER TABLE public.case_unit OWNER TO postgres;

--
-- Name: TABLE case_unit; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.case_unit IS 'Занятие';


--
-- Name: case_unit_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.case_unit_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.case_unit_id_seq OWNER TO postgres;

--
-- Name: case_unit_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.case_unit_id_seq OWNED BY public.case_unit.id;


--
-- Name: child_achievement_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.child_achievement_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.child_achievement_id_seq OWNER TO postgres;

--
-- Name: child_achievement_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.child_achievement_id_seq OWNED BY public.achievement_children.id;


--
-- Name: city; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.city (
    id bigint NOT NULL,
    name character varying(64) NOT NULL,
    region_id bigint NOT NULL
);


ALTER TABLE public.city OWNER TO postgres;

--
-- Name: TABLE city; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.city IS 'Город';


--
-- Name: city_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.city_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.city_id_seq OWNER TO postgres;

--
-- Name: city_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.city_id_seq OWNED BY public.city.id;


--
-- Name: contact_person; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.contact_person (
    id integer NOT NULL,
    fullname character varying(200) NOT NULL,
    phone character varying(20) NOT NULL,
    relation character varying(50) NOT NULL,
    user_id integer NOT NULL
);


ALTER TABLE public.contact_person OWNER TO postgres;

--
-- Name: TABLE contact_person; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.contact_person IS 'Контактное лицо ученика';


--
-- Name: contact_person_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.contact_person_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.contact_person_id_seq OWNER TO postgres;

--
-- Name: contact_person_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.contact_person_id_seq OWNED BY public.contact_person.id;


--
-- Name: date_substitution_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.date_substitution_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.date_substitution_id_seq OWNER TO postgres;

--
-- Name: date_substitution; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.date_substitution (
    id bigint DEFAULT nextval('public.date_substitution_id_seq'::regclass) NOT NULL,
    target_date date NOT NULL,
    substitute_date date,
    city_id bigint NOT NULL,
    timing_grid_id integer
);


ALTER TABLE public.date_substitution OWNER TO postgres;

--
-- Name: TABLE date_substitution; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.date_substitution IS 'Праздники и переносы рабочих дней';


--
-- Name: learning_group; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.learning_group (
    id integer NOT NULL,
    title character varying(100) NOT NULL,
    quantum_id integer NOT NULL,
    teacher_id integer NOT NULL,
    substitute_teacher_id integer NOT NULL,
    min_students smallint DEFAULT 1 NOT NULL,
    max_students smallint NOT NULL,
    start_date date NOT NULL,
    finish_date date,
    min_age smallint DEFAULT 6 NOT NULL,
    optional boolean DEFAULT false NOT NULL,
    timing_grid_id integer
);


ALTER TABLE public.learning_group OWNER TO postgres;

--
-- Name: TABLE learning_group; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.learning_group IS 'Учебная группа';


--
-- Name: COLUMN learning_group.timing_grid_id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.learning_group.timing_grid_id IS 'Особое расписание звонков';


--
-- Name: learning_group_case; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.learning_group_case (
    id integer NOT NULL,
    learning_group_id integer NOT NULL,
    case_id integer NOT NULL,
    state smallint DEFAULT 0 NOT NULL
);


ALTER TABLE public.learning_group_case OWNER TO postgres;

--
-- Name: TABLE learning_group_case; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.learning_group_case IS 'Кейсы группы';


--
-- Name: learning_group_case_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.learning_group_case_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.learning_group_case_id_seq OWNER TO postgres;

--
-- Name: learning_group_case_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.learning_group_case_id_seq OWNED BY public.learning_group_case.id;


--
-- Name: learning_group_hours; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.learning_group_hours (
    id integer NOT NULL,
    day_of_week smallint NOT NULL,
    order_num smallint NOT NULL,
    learning_group_id integer
);


ALTER TABLE public.learning_group_hours OWNER TO postgres;

--
-- Name: COLUMN learning_group_hours.order_num; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.learning_group_hours.order_num IS 'Порядковый номер урока';


--
-- Name: learning_group_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.learning_group_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.learning_group_id_seq OWNER TO postgres;

--
-- Name: learning_group_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.learning_group_id_seq OWNED BY public.learning_group.id;


--
-- Name: lesson; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.lesson (
    id integer NOT NULL,
    learning_group_id integer NOT NULL,
    lesson_date date DEFAULT ('now'::text)::date NOT NULL,
    lesson_time time without time zone NOT NULL,
    held boolean DEFAULT true NOT NULL,
    teacher_id integer
);


ALTER TABLE public.lesson OWNER TO postgres;

--
-- Name: TABLE lesson; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.lesson IS 'Урок группы';


--
-- Name: lesson_course_hours_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.lesson_course_hours_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.lesson_course_hours_id_seq OWNER TO postgres;

--
-- Name: lesson_course_hours_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.lesson_course_hours_id_seq OWNED BY public.learning_group_hours.id;


--
-- Name: lesson_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.lesson_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.lesson_id_seq OWNER TO postgres;

--
-- Name: lesson_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.lesson_id_seq OWNED BY public.lesson.id;


--
-- Name: notice; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.notice (
    id bigint NOT NULL,
    code integer NOT NULL,
    dt timestamp without time zone DEFAULT now() NOT NULL,
    receiver_id integer NOT NULL,
    description character varying(1024) NOT NULL,
    viewed boolean DEFAULT false NOT NULL,
    sender_id integer
);


ALTER TABLE public.notice OWNER TO postgres;

--
-- Name: notice_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.notice_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.notice_id_seq OWNER TO postgres;

--
-- Name: notice_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.notice_id_seq OWNED BY public.notice.id;


--
-- Name: notice_type; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.notice_type (
    id integer NOT NULL,
    pattern character varying(1000) NOT NULL,
    class character varying(100) NOT NULL,
    title character varying(100) NOT NULL
);


ALTER TABLE public.notice_type OWNER TO postgres;

--
-- Name: quantum; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.quantum (
    id bigint NOT NULL,
    name character varying(256) NOT NULL,
    icon character varying(64),
    city_id bigint NOT NULL,
    description character varying(1024) NOT NULL
);


ALTER TABLE public.quantum OWNER TO postgres;

--
-- Name: TABLE quantum; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.quantum IS 'Кванториум';


--
-- Name: quantum_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.quantum_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.quantum_id_seq OWNER TO postgres;

--
-- Name: quantum_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.quantum_id_seq OWNED BY public.quantum.id;


--
-- Name: region; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.region (
    id bigint NOT NULL,
    name character varying(128) NOT NULL,
    timezone smallint
);


ALTER TABLE public.region OWNER TO postgres;

--
-- Name: TABLE region; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.region IS 'Область';


--
-- Name: region_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.region_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.region_id_seq OWNER TO postgres;

--
-- Name: region_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.region_id_seq OWNED BY public.region.id;


--
-- Name: report; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.report (
    id bigint NOT NULL,
    city_id bigint NOT NULL,
    title character varying(200) NOT NULL,
    template_url character varying(100),
    sql_template character varying(1000),
    form_url character varying(100)
);


ALTER TABLE public.report OWNER TO postgres;

--
-- Name: report_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.report_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.report_id_seq OWNER TO postgres;

--
-- Name: report_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.report_id_seq OWNED BY public.report.id;


--
-- Name: structure_version_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.structure_version_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.structure_version_seq OWNER TO postgres;

--
-- Name: sys_params; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.sys_params (
    id bigint NOT NULL,
    db_version integer DEFAULT 0 NOT NULL
);


ALTER TABLE public.sys_params OWNER TO postgres;

--
-- Name: sysuser; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.sysuser (
    id bigint NOT NULL,
    username character varying(32) NOT NULL,
    pswd character varying(32) NOT NULL,
    first_name character varying(50) NOT NULL,
    last_name character varying(100) NOT NULL,
    birth_date date NOT NULL,
    gender boolean NOT NULL,
    created date DEFAULT ('now'::text)::date NOT NULL,
    last_login date,
    active boolean DEFAULT false NOT NULL,
    city_id bigint NOT NULL,
    sysuser_type_id smallint NOT NULL,
    second_name character varying(50),
    speciality character varying(512),
    phone character varying(20),
    actual_address character varying(200),
    passport_number character varying(6),
    passport_series character varying(5),
    passport_issue_date date,
    passport_issue_authority character varying(200),
    passport_address character varying(200),
    passport_department_code character varying(8),
    personnel_number character varying(20),
    staffer boolean,
    shift smallint,
    balance smallint DEFAULT 0 NOT NULL,
    disable_reason character varying(200),
    email character varying(100),
    has_social_benefits boolean DEFAULT false NOT NULL,
    social_benefits_comment character varying(200),
    document_type smallint DEFAULT 0 NOT NULL,
    CONSTRAINT chk_sysuser_shift CHECK (((shift > 0) AND (shift <= 2)))
);


ALTER TABLE public.sysuser OWNER TO postgres;

--
-- Name: COLUMN sysuser.shift; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.sysuser.shift IS 'смена';


--
-- Name: COLUMN sysuser.balance; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.sysuser.balance IS 'Баланс QC';


--
-- Name: COLUMN sysuser.disable_reason; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.sysuser.disable_reason IS 'Причина деактивации';


--
-- Name: COLUMN sysuser.document_type; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.sysuser.document_type IS 'Тип удостоверяющего документа';


--
-- Name: sysuser_type; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.sysuser_type (
    id smallint NOT NULL,
    name character varying(50) NOT NULL
);


ALTER TABLE public.sysuser_type OWNER TO postgres;

--
-- Name: task; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.task (
    id bigint NOT NULL,
    name character varying(256) NOT NULL,
    max_score smallint NOT NULL,
    sort_id smallint NOT NULL,
    description character varying(1024),
    case_id integer NOT NULL
);


ALTER TABLE public.task OWNER TO postgres;

--
-- Name: TABLE task; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.task IS 'Задание';


--
-- Name: task_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.task_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.task_id_seq OWNER TO postgres;

--
-- Name: task_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.task_id_seq OWNED BY public.task.id;


--
-- Name: teacher_quantum; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.teacher_quantum (
    id bigint NOT NULL,
    teacher_id bigint NOT NULL,
    quantum_id bigint NOT NULL
);


ALTER TABLE public.teacher_quantum OWNER TO postgres;

--
-- Name: TABLE teacher_quantum; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.teacher_quantum IS 'Квантумы, к которым есть доступ учителю';


--
-- Name: teacher_quantum_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.teacher_quantum_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.teacher_quantum_id_seq OWNER TO postgres;

--
-- Name: teacher_quantum_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.teacher_quantum_id_seq OWNED BY public.teacher_quantum.id;


--
-- Name: timing_grid_ver_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.timing_grid_ver_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.timing_grid_ver_seq OWNER TO postgres;

--
-- Name: timing_grid; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.timing_grid (
    id integer NOT NULL,
    name character varying(100) NOT NULL,
    city_id bigint NOT NULL,
    duration smallint DEFAULT 40 NOT NULL,
    city_default boolean DEFAULT false NOT NULL,
    version bigint DEFAULT nextval('public.timing_grid_ver_seq'::regclass) NOT NULL
);


ALTER TABLE public.timing_grid OWNER TO postgres;

--
-- Name: TABLE timing_grid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.timing_grid IS 'Расписание звонков';


--
-- Name: timing_grid_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.timing_grid_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.timing_grid_id_seq OWNER TO postgres;

--
-- Name: timing_grid_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.timing_grid_id_seq OWNED BY public.timing_grid.id;


--
-- Name: timing_grid_unit; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.timing_grid_unit (
    id integer NOT NULL,
    timing_grid_id integer NOT NULL,
    day_type smallint DEFAULT 0 NOT NULL,
    order_num smallint NOT NULL,
    begin_time time without time zone NOT NULL
);


ALTER TABLE public.timing_grid_unit OWNER TO postgres;

--
-- Name: COLUMN timing_grid_unit.day_type; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.timing_grid_unit.day_type IS 'Тип дня';


--
-- Name: timing_grid_unit_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.timing_grid_unit_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.timing_grid_unit_id_seq OWNER TO postgres;

--
-- Name: timing_grid_unit_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.timing_grid_unit_id_seq OWNED BY public.timing_grid_unit.id;


--
-- Name: user_achievement; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.user_achievement (
    id integer NOT NULL,
    user_id integer NOT NULL,
    achievement_id integer NOT NULL,
    expire_date date
);


ALTER TABLE public.user_achievement OWNER TO postgres;

--
-- Name: TABLE user_achievement; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.user_achievement IS 'Достижения ученика';


--
-- Name: user_achievement_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.user_achievement_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.user_achievement_id_seq OWNER TO postgres;

--
-- Name: user_achievement_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.user_achievement_id_seq OWNED BY public.user_achievement.id;


--
-- Name: user_case; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.user_case (
    id integer NOT NULL,
    user_id integer NOT NULL,
    case_id integer NOT NULL,
    finished boolean DEFAULT false NOT NULL,
    passed boolean DEFAULT false NOT NULL
);


ALTER TABLE public.user_case OWNER TO postgres;

--
-- Name: TABLE user_case; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.user_case IS 'Запись учеников на кейсы';


--
-- Name: user_case_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.user_case_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.user_case_id_seq OWNER TO postgres;

--
-- Name: user_case_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.user_case_id_seq OWNED BY public.user_case.id;


--
-- Name: user_learning_group; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.user_learning_group (
    id integer NOT NULL,
    user_id integer NOT NULL,
    learning_group_id integer NOT NULL
);


ALTER TABLE public.user_learning_group OWNER TO postgres;

--
-- Name: TABLE user_learning_group; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.user_learning_group IS 'Связь учеников и групп';


--
-- Name: user_learning_group_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.user_learning_group_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.user_learning_group_id_seq OWNER TO postgres;

--
-- Name: user_learning_group_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.user_learning_group_id_seq OWNED BY public.user_learning_group.id;


--
-- Name: user_lesson; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.user_lesson (
    id bigint NOT NULL,
    user_id bigint NOT NULL,
    score smallint NOT NULL,
    lesson_id integer NOT NULL,
    case_id integer
);


ALTER TABLE public.user_lesson OWNER TO postgres;

--
-- Name: TABLE user_lesson; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.user_lesson IS 'Пройденные занятия';


--
-- Name: user_lesson_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.user_lesson_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.user_lesson_id_seq OWNER TO postgres;

--
-- Name: user_lesson_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.user_lesson_id_seq OWNED BY public.user_lesson.id;


--
-- Name: user_task; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.user_task (
    id bigint NOT NULL,
    user_id bigint NOT NULL,
    task_id bigint NOT NULL,
    score smallint NOT NULL,
    completed boolean DEFAULT false NOT NULL
);


ALTER TABLE public.user_task OWNER TO postgres;

--
-- Name: TABLE user_task; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.user_task IS 'Выполненные задания';


--
-- Name: user_task_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.user_task_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.user_task_id_seq OWNER TO postgres;

--
-- Name: user_task_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.user_task_id_seq OWNED BY public.user_task.id;


--
-- Name: users_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.users_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.users_id_seq OWNER TO postgres;

--
-- Name: users_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.users_id_seq OWNED BY public.sysuser.id;


--
-- Name: achievement id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.achievement ALTER COLUMN id SET DEFAULT nextval('public.achievement_id_seq'::regclass);


--
-- Name: achievement_children id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.achievement_children ALTER COLUMN id SET DEFAULT nextval('public.child_achievement_id_seq'::regclass);


--
-- Name: achievement_group id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.achievement_group ALTER COLUMN id SET DEFAULT nextval('public.achievement_group_id_seq'::regclass);


--
-- Name: action_log id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.action_log ALTER COLUMN id SET DEFAULT nextval('public.action_log_id_seq'::regclass);


--
-- Name: case id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."case" ALTER COLUMN id SET DEFAULT nextval('public.case_id_seq'::regclass);


--
-- Name: case_achievement id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_achievement ALTER COLUMN id SET DEFAULT nextval('public.case_achievement_id_seq'::regclass);


--
-- Name: case_required_achievement id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_required_achievement ALTER COLUMN id SET DEFAULT nextval('public.case_required_achievement_id_seq'::regclass);


--
-- Name: case_unit id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_unit ALTER COLUMN id SET DEFAULT nextval('public.case_unit_id_seq'::regclass);


--
-- Name: city id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.city ALTER COLUMN id SET DEFAULT nextval('public.city_id_seq'::regclass);


--
-- Name: contact_person id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.contact_person ALTER COLUMN id SET DEFAULT nextval('public.contact_person_id_seq'::regclass);


--
-- Name: learning_group id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.learning_group ALTER COLUMN id SET DEFAULT nextval('public.learning_group_id_seq'::regclass);


--
-- Name: learning_group_case id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.learning_group_case ALTER COLUMN id SET DEFAULT nextval('public.learning_group_case_id_seq'::regclass);


--
-- Name: learning_group_hours id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.learning_group_hours ALTER COLUMN id SET DEFAULT nextval('public.lesson_course_hours_id_seq'::regclass);


--
-- Name: lesson id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.lesson ALTER COLUMN id SET DEFAULT nextval('public.lesson_id_seq'::regclass);


--
-- Name: notice id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.notice ALTER COLUMN id SET DEFAULT nextval('public.notice_id_seq'::regclass);


--
-- Name: quantum id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.quantum ALTER COLUMN id SET DEFAULT nextval('public.quantum_id_seq'::regclass);


--
-- Name: region id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.region ALTER COLUMN id SET DEFAULT nextval('public.region_id_seq'::regclass);


--
-- Name: report id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.report ALTER COLUMN id SET DEFAULT nextval('public.report_id_seq'::regclass);


--
-- Name: sysuser id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.sysuser ALTER COLUMN id SET DEFAULT nextval('public.users_id_seq'::regclass);


--
-- Name: task id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.task ALTER COLUMN id SET DEFAULT nextval('public.task_id_seq'::regclass);


--
-- Name: teacher_quantum id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.teacher_quantum ALTER COLUMN id SET DEFAULT nextval('public.teacher_quantum_id_seq'::regclass);


--
-- Name: test id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.test ALTER COLUMN id SET DEFAULT nextval('public.test_id_seq'::regclass);


--
-- Name: timing_grid id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.timing_grid ALTER COLUMN id SET DEFAULT nextval('public.timing_grid_id_seq'::regclass);


--
-- Name: timing_grid_unit id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.timing_grid_unit ALTER COLUMN id SET DEFAULT nextval('public.timing_grid_unit_id_seq'::regclass);


--
-- Name: user_achievement id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_achievement ALTER COLUMN id SET DEFAULT nextval('public.user_achievement_id_seq'::regclass);


--
-- Name: user_case id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_case ALTER COLUMN id SET DEFAULT nextval('public.user_case_id_seq'::regclass);


--
-- Name: user_learning_group id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_learning_group ALTER COLUMN id SET DEFAULT nextval('public.user_learning_group_id_seq'::regclass);


--
-- Name: user_lesson id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_lesson ALTER COLUMN id SET DEFAULT nextval('public.user_lesson_id_seq'::regclass);


--
-- Name: user_task id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_task ALTER COLUMN id SET DEFAULT nextval('public.user_task_id_seq'::regclass);


--
-- Data for Name: achievement; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.achievement (id, title, achievement_group_id, description, valid_months) VALUES (2, 'Виды крепления деталей', 1, NULL, 3);
INSERT INTO public.achievement (id, title, achievement_group_id, description, valid_months) VALUES (6, 'История авиации', 4, 'тест', 3);
INSERT INTO public.achievement (id, title, achievement_group_id, description, valid_months) VALUES (5, 'Пилотирование уровень 1', 3, NULL, 3);
INSERT INTO public.achievement (id, title, achievement_group_id, description, valid_months) VALUES (1, 'Материалы авиастроения', 1, NULL, 3);
INSERT INTO public.achievement (id, title, achievement_group_id, description, valid_months) VALUES (4, 'Форма крыла', 2, NULL, 3);
INSERT INTO public.achievement (id, title, achievement_group_id, description, valid_months) VALUES (3, 'Подъемные силы', 2, NULL, 3);


--
-- Data for Name: achievement_children; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.achievement_children (id, achievement_id, child_id) VALUES (24, 5, 1);
INSERT INTO public.achievement_children (id, achievement_id, child_id) VALUES (25, 5, 4);
INSERT INTO public.achievement_children (id, achievement_id, child_id) VALUES (29, 2, 3);
INSERT INTO public.achievement_children (id, achievement_id, child_id) VALUES (30, 5, 2);
INSERT INTO public.achievement_children (id, achievement_id, child_id) VALUES (34, 1, 6);
INSERT INTO public.achievement_children (id, achievement_id, child_id) VALUES (52, 2, 6);


--
-- Data for Name: achievement_group; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.achievement_group (id, title, parent_id, city_id) VALUES (1, 'Конструктирование квадракоптера', 3, 1);
INSERT INTO public.achievement_group (id, title, parent_id, city_id) VALUES (2, 'Теория аэродинамики', 3, 1);
INSERT INTO public.achievement_group (id, title, parent_id, city_id) VALUES (4, 'Пилотирование АС', NULL, 1);
INSERT INTO public.achievement_group (id, title, parent_id, city_id) VALUES (3, 'Пилотирование коптера', 4, 1);
INSERT INTO public.achievement_group (id, title, parent_id, city_id) VALUES (6, 'Основы робототехники', NULL, 1);
INSERT INTO public.achievement_group (id, title, parent_id, city_id) VALUES (5, 'Теория конечных автоматов', 6, 1);
INSERT INTO public.achievement_group (id, title, parent_id, city_id) VALUES (7, 'test', 1, 1);


--
-- Data for Name: action_log; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (1, '2019-04-29 22:53:45.454702', 'f6bdfe6a-f3a4-44c3-9933-d0a0464b070d', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 5');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (2, '2019-04-29 22:54:50.589428', '826a2f76-8548-42ab-9bb4-233d63bc9d4d', 1, 'sysuser_update', false, 'ОШИБКА:  значение не умещается в тип character varying(4)
(22001)');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (3, '2019-04-29 22:58:06.923657', 'de83d529-9036-43c2-a32d-4df59d9f4d69', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 5');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (4, '2019-05-25 11:07:26.077546', '9dfd4514-9bcf-4896-ada9-9d1bb4cd238b', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (5, '2019-05-25 12:07:20.782836', '29cfbbab-e324-4a27-90f9-e298bf6023e2', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (6, '2019-05-25 12:07:33.857579', '22e4e6f6-5584-451c-b018-4a0ce9a92e8a', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (7, '2019-05-25 12:09:09.787041', '171b499b-d473-4d01-ac47-3ec8edddd6fa', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -1');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (8, '2019-05-25 12:09:09.787041', '356a9c06-0de3-4c96-bdeb-229c95228732', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -2');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (9, '2019-05-25 12:09:09.895047', '9f84698f-ecee-44d8-95c6-2b43312c47a6', 1, 'contact_person_insert', true, 'В таблицу contact_person добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (10, '2019-05-25 14:17:17.519555', '8f87bd1e-ea60-470f-b6b4-5b8c8b4ff251', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 5');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (11, '2019-05-25 14:18:25.89341', '35591a62-f1fa-4c5c-9fce-c80eb69a0294', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 5');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (12, '2019-05-25 14:33:16.217941', '8a57e983-a7eb-4e47-ba4c-3dbc60e4f4d3', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (13, '2019-05-25 14:33:45.799633', 'e13016ee-3cf1-46de-85ff-a052b219a852', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 13');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (14, '2019-05-25 14:34:21.112653', '528a37b9-d476-46d1-9d49-397df1a0113b', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (15, '2019-05-25 14:34:27.484017', 'd3f4947d-ea5e-47a1-8a22-99ee62a3d9f9', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 13');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (16, '2019-05-25 14:34:44.950016', 'bff74944-1333-46e8-ad14-371ce6fc3c24', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 13');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (17, '2019-05-25 14:36:30.213037', '22950fa0-9b6e-4182-9f0b-c18d6bf28953', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (18, '2019-05-25 14:36:32.343159', 'ffae93a4-f1ee-4d92-a1d5-018d5a233d84', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 13');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (19, '2019-05-25 14:36:41.155663', '64d734ad-4bcd-4818-b320-2e8426146775', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (20, '2019-05-25 14:38:41.060521', 'd8279417-901c-46c4-95b8-8eb04c4fd06f', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (21, '2019-05-25 14:47:43.565551', 'b38dfb15-7484-4b0b-a2ee-e73a5800ca05', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 16');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (22, '2019-05-25 14:47:45.920685', '62b2cdf2-39ff-45db-a641-48b98542c78a', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 15');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (23, '2019-05-25 14:47:54.116154', 'e15f572d-9eb9-44aa-b6e3-27a0136918c3', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (24, '2019-05-25 14:48:03.543693', '88b2b323-87e8-40a1-81e0-87a7cbe6411b', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (25, '2019-05-25 21:29:10.036027', '3cb6a4a8-73b5-44e8-a12b-a8119b0d51ab', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 18');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (26, '2019-05-25 21:29:12.908192', 'c6699fd9-64d2-4351-b237-6648d3ab5ba4', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 17');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (27, '2019-05-25 21:29:27.769042', '10ae3da2-646c-47bd-96c6-8205b07cefd0', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (28, '2019-05-25 21:29:52.250442', '4b7bf4ed-56d6-439b-b96a-f2d8fcb107d5', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (29, '2019-05-25 21:31:02.012432', '2941bafa-c3f6-4859-8079-e8cfd1c995dd', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 20');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (30, '2019-05-25 21:41:02.352699', 'abe601aa-1c6a-4646-9cd5-d649995e3362', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 20');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (31, '2019-05-25 21:41:04.969847', 'ebbd8cbc-e1c9-4f83-99cb-19cb3d27faaa', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 19');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (32, '2019-05-25 21:41:19.161648', '5f93dfbc-81ba-4fa9-bbd0-0f89db0bbeca', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (33, '2019-05-25 21:41:43.496012', 'e4d2c80a-333f-4321-b93f-68ffd89f8e19', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 21');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (34, '2019-05-25 21:51:02.124862', 'ac53dba6-3576-4f10-9d1a-3d417e645911', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (35, '2019-05-25 21:51:16.24867', 'b1b024e4-0836-46ef-b5e0-387e7640ad34', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 21');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (36, '2019-05-25 21:54:44.622589', '7db22449-9d30-4526-ae21-867eb001fee1', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 22');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (37, '2019-05-25 21:54:47.732767', '84e52420-b6bf-421a-af79-428865a0f841', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 21');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (38, '2019-05-25 21:55:00.234482', '1a407121-e76a-4981-a53d-712bb2e6a799', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (39, '2019-05-25 21:55:13.826259', '8a6a3c12-c5a0-47d2-acd3-f7472735b4d3', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (40, '2019-05-25 21:55:36.091532', '68a99c38-6753-4cb9-b5ce-7eb073ff14c8', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 23');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (41, '2019-05-25 21:55:48.27823', '7253dec6-629b-4b30-8bd8-be67b95141dd', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (42, '2019-05-25 21:55:59.171853', '42b8b804-19a5-4afb-916a-fd3de678c84c', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 25');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (43, '2019-05-25 21:56:02.264029', '2f17ce71-2a63-4938-9acd-655ec9187f16', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 24');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (44, '2019-05-25 21:56:04.604163', 'a3fe6bba-15b1-41d7-a0c3-b420e3ed2ac4', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 23');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (45, '2019-05-25 22:41:30.418071', 'e95a9692-5037-4ac6-9c83-79d6e5986bf0', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (46, '2019-05-25 22:41:43.33081', '2a16be14-2f4f-440d-b525-2ad972aebf3a', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 26');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (47, '2019-05-25 22:41:52.458332', '1a0c7204-86b5-4cbb-88f0-b90d2a32fd5e', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 26');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (48, '2019-05-25 22:42:04.792037', 'f3b0dce8-d567-44f2-a10f-2969c8c4ce6f', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 26');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (49, '2019-05-25 22:42:25.159202', '6313544c-8f7a-4e40-88a7-93792602c040', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 26');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (50, '2019-05-25 22:43:03.694406', 'c4de2aae-2167-4ef3-aca4-a2016ae3f8a9', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (51, '2019-05-25 22:43:32.562057', '6b868b37-b432-43d2-a9c1-ea337aad415a', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (52, '2019-05-25 22:43:42.785642', 'f8b8aa85-da1e-41b2-835d-955a38b8c722', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (53, '2019-05-25 23:02:16.526344', '18169991-686f-4205-817d-c28f2d8ec52b', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (54, '2019-05-25 23:02:29.096063', '061e6f0a-8177-46ef-bf83-372c83b7b6a6', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (55, '2019-05-25 23:03:33.071722', '21b09946-f2ca-44f9-9196-8c5116d8c1ed', 1, 'sysuser_insert', true, 'В таблицу sysuser добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (56, '2019-05-25 23:03:38.374026', '1af0dfc0-bbf8-40ea-b3e5-dfec1ef66b45', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (57, '2019-05-25 23:03:52.213817', '9ffd49c5-21b5-4b0d-ab08-1c0ee9d3a1f0', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 12');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (58, '2019-05-25 23:05:51.824659', '5dc6446c-aeec-4a26-ae7a-a12d9453443b', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (59, '2019-05-25 23:06:13.990926', '64a08ee6-bea8-4906-8400-f5725848c705', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (60, '2019-05-25 23:06:27.767714', 'ee76d4d7-6f3e-48a3-9312-791c2269ab93', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 27');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (61, '2019-05-25 23:06:42.663566', '664d7adf-5d8d-4375-b3a5-8c49cb9f594f', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 28');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (62, '2019-05-25 23:06:45.432725', '7cf6b577-7ab6-436d-99b4-21b71d53d9c6', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 27');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (63, '2019-05-28 21:09:45.958491', 'da401703-44f1-4b86-81b6-7419de6f37cd', 1, 'learning_group_update', true, 'В таблице learning_group отредактирована запись с идентификатором 1');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (64, '2019-05-28 21:10:51.147188', '8cb2bde9-f1ed-4388-8f90-c3af9cf49a32', 1, 'learning_group_update', true, 'В таблице learning_group отредактирована запись с идентификатором 1');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (65, '2019-05-28 21:12:27.130646', '47c4c4b2-9dbb-474a-beeb-1db10c56bda8', 1, 'learning_group_insert', true, 'В таблицу learning_group добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (66, '2019-05-30 14:29:07.170259', '1f51b097-d34c-4bbe-a437-5e886c76b7e1', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (67, '2019-05-30 14:29:20.759031', '21549d04-f104-4098-9420-a5f72833e5b3', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (68, '2019-05-30 14:31:45.587245', 'bdf862a7-aaa0-4bcd-ba62-eb2675356e68', 1, 'sysuser_insert', true, 'В таблицу sysuser добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (69, '2019-05-30 14:31:49.462463', 'f4dbe581-b183-4bcb-9b3d-508f6ed64340', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (70, '2019-05-30 14:31:58.817995', 'c4533604-a9c5-4412-88da-68a908bbe6bf', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 13');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (71, '2019-05-30 21:28:40.073791', 'a7775355-4ef0-4e05-a68d-f8b728cc37f7', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 13');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (72, '2019-05-30 21:28:53.962585', '712f66a4-2827-4433-8ea3-c614c48a4b02', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 13');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (73, '2019-05-30 21:29:21.427156', 'c6eeb69e-984b-43a8-89ac-491f2ed068e0', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 13');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (74, '2019-05-30 21:29:48.403699', '48181c64-b6c1-4c1a-a640-9c6074fd78ff', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 13');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (75, '2019-05-30 21:29:55.790121', 'c4fa0e97-b62d-43ee-b163-fa855b156de6', 1, 'sysuser_delete', true, 'Из таблицы sysuser удалена запись c идентификатором 13');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (76, '2019-05-30 21:30:59.918789', 'f539d955-b259-4f39-b033-f557e411177e', 1, 'sysuser_insert', true, 'В таблицу sysuser добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (77, '2019-05-30 22:58:51.432674', '33400bc9-0ef9-49d4-8bea-c052bbe67b17', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (78, '2019-05-30 23:10:44.201338', 'af8ad261-c330-4305-917d-f7b9bb5e1fc4', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (79, '2019-05-30 23:11:07.822684', '4c16a33a-77f8-497e-b23f-29640c69e589', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (80, '2019-05-30 23:19:59.944051', '220b2451-a96d-4e84-b369-717d35d6dd53', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (81, '2019-05-30 23:20:19.144141', '9411be71-82e7-4230-b045-dd35c11f4dca', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 29');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (82, '2019-05-30 23:38:48.912517', 'ddc5a2f2-c15b-4c1b-ae5e-18e202b31d3b', 1, 'learning_group_update', true, 'В таблице learning_group отредактирована запись с идентификатором 4');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (83, '2019-05-30 23:39:25.915629', '69704d2b-a5ff-42ff-80dd-d3aaafb119ba', 1, 'learning_group_insert', true, 'В таблицу learning_group добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (84, '2019-05-30 23:39:52.389143', 'dfff9b8e-9a49-4d99-8b81-569c5217ffc0', 1, 'learning_group_update', false, 'ОШИБКА:  нулевое значение в столбце "teacher_id" нарушает ограничение NOT NULL
DETAIL:  Ошибочная строка содержит (4, QQQQQ, 1, null, 4, 3, 20, 2019-05-28, 2019-08-26, 4, f, null).
(23502)');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (85, '2019-05-30 23:40:11.107214', 'c5befcd4-8de5-4b4c-85f0-dca845e9d3af', 1, 'learning_group_update', true, 'В таблице learning_group отредактирована запись с идентификатором 5');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (86, '2019-05-31 00:26:25.092566', 'ef0bf990-a5eb-4241-9dd7-6b723be3375f', 1, 'case_update', true, 'В таблице case отредактирована запись с идентификатором 1');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (87, '2019-05-31 00:26:36.303203', '1e2d7d28-da3d-4be0-ab2f-cf0cc489f439', 1, 'case_update', true, 'В таблице case отредактирована запись с идентификатором 1');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (88, '2019-05-31 00:30:18.999823', '97a239a7-e009-4cf9-bf45-2b784f4473db', 1, 'case_insert', true, 'В таблицу case добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (89, '2019-05-31 00:31:46.055729', 'e472827b-7a6a-459f-a644-d4d09dcc8483', 1, 'case_unit_insert', true, 'В таблицу case_unit добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (90, '2019-05-31 00:31:53.667149', 'a4935adb-75ad-40c5-82eb-cbfc13381f08', 1, 'case_unit_update', true, 'В таблице case_unit отредактирована запись с идентификатором 55');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (91, '2019-05-31 18:24:57.758683', 'a035e4d1-41b1-4ed1-95fa-b0ee8e813a6b', 1, 'learning_group_update', true, 'В таблице learning_group отредактирована запись с идентификатором 5');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (92, '2019-05-31 18:25:26.71632', 'fa212967-4691-40bd-a4d5-59e6189de8b2', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (93, '2019-05-31 18:25:36.340867', '37d69c17-ff69-451e-a3ed-7feaa83f6cb1', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (94, '2019-05-31 18:25:55.214943', '382e5923-a529-4078-a580-9e5225d6823c', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (95, '2019-05-31 18:26:20.265368', 'c51c1bef-e9a6-4b4d-80f7-5bc68094dc34', 1, 'case_update', true, 'В таблице case отредактирована запись с идентификатором 18');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (96, '2019-05-31 18:26:47.309906', '6be8a814-3ace-44fd-bda9-6b59c1897d0d', 1, 'case_update', true, 'В таблице case отредактирована запись с идентификатором 18');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (97, '2019-05-31 18:27:08.456105', '00710681-9bc0-4eb9-83de-6ab5324d724d', 1, 'case_insert', true, 'В таблицу case добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (98, '2019-05-31 18:27:45.617207', 'c5e449b0-f9c7-45d7-ad10-a6184d9042d6', 1, 'case_delete', true, 'Из таблицы case удалена запись c идентификатором 19');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (99, '2019-05-31 18:45:13.456601', 'ba6bb628-c7eb-4936-8f45-84910c063510', 1, 'learning_group_insert', true, 'В таблицу learning_group добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (100, '2019-05-31 18:45:46.009438', '35f0f4c0-2a12-4157-8190-f6d4f7e230c9', 1, 'learning_group_update', true, 'В таблице learning_group отредактирована запись с идентификатором 6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (101, '2019-05-31 18:55:46.596983', 'fbb1603e-a640-464d-89d5-caf9d5b4e7b1', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (102, '2019-05-31 18:56:09.411277', '11594617-c306-4fdf-a4aa-92b62288d0d6', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 30');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (103, '2019-05-31 18:56:26.474244', 'e1bcb5dd-9bf6-46fa-a28e-6dba84f0de74', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (104, '2019-05-31 18:56:42.601158', 'cc5dedce-2d89-40c8-8776-acea8cf1aca5', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (105, '2019-05-31 18:56:53.675785', '131890c9-9787-436d-9837-b9c4d39c7a67', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 32');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (106, '2019-05-31 18:57:40.719446', '11ee7c2b-4ad4-41b3-9226-59192406fdc6', 1, 'case_insert', true, 'В таблицу case добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (107, '2019-05-31 19:38:04.24631', '074f2a5a-b10f-49d3-abb8-4a718b24e955', 1, 'timing_grid_insert', true, 'В таблицу timing_grid добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (108, '2019-05-31 19:38:30.098771', 'a3e30e43-bf22-4c40-ba5a-b48a6d6f9c47', 1, 'timing_grid_update', true, 'В таблице timing_grid отредактирована запись с идентификатором 18');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (109, '2019-05-31 19:38:30.193777', '7114ee46-8ee6-4eed-8be8-b07253d4b8a3', 1, 'batch_submit', true, 'Success');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (110, '2019-05-31 19:39:01.242534', 'f3514c6a-9574-4e19-8252-817513689b13', 1, 'get_achievement_group_structure', true, 'Иерархическое содержание группы достижений');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (111, '2019-05-31 19:40:22.876157', 'cc5695f2-75fd-4d72-8749-9a9457d739d5', 1, 'learning_group_delete', true, 'Из таблицы learning_group удалена запись c идентификатором 6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (112, '2019-05-31 19:40:26.839379', 'af5d833d-4ed2-4783-a1e6-6239fde4bd2f', 1, 'learning_group_delete', true, 'Из таблицы learning_group удалена запись c идентификатором 5');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (113, '2019-05-31 23:33:23.758109', '8e12b1fb-051d-4d27-99d8-71c83f4a63d6', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 32');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (114, '2019-05-31 23:33:25.90823', 'da3c2dd8-f77f-460f-8f20-4c3af8f624d3', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 31');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (115, '2019-05-31 23:33:28.127357', 'd68d6eb0-f5c3-4db4-bc6c-3dfcebbd930e', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 30');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (116, '2019-05-31 23:33:33.997689', 'd4455c89-e709-41ed-ab95-a90c16403b28', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (117, '2019-06-03 10:51:51.096305', '8fa4ddff-145f-4df3-90ce-623877906f5b', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (118, '2019-06-04 16:19:21.226765', '09b72324-f33e-4904-9a52-849704d6e100', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (119, '2019-06-04 16:19:33.899482', '9304a58a-cef1-434e-9b5f-96d927a65424', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 33');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (120, '2019-06-04 17:04:56.739079', 'c8f4a03a-996c-4629-927f-8e81fe9c40aa', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 33');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (121, '2019-06-04 17:18:40.99339', '1c24d896-10e2-44e8-9348-c8a441a6d98f', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (122, '2019-06-04 17:18:50.865954', '09393847-b650-424c-bec5-1d3875f201c3', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 34');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (123, '2019-06-04 17:19:22.808773', '80cff19d-fd16-46a0-889f-40e79ffb802d', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 34');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (124, '2019-06-04 17:19:40.445769', '87d9548a-9a99-42a8-b506-9312029e036d', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 34');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (125, '2019-06-06 15:08:11.374667', '6c019a0c-8f8e-4600-bcfc-9eea41e7e100', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 34');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (126, '2019-06-06 15:08:26.156687', '008f8c6c-2448-4967-ae7f-c7e87fdc3b66', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 34');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (127, '2019-06-06 15:08:40.317707', '9975d82b-63e7-4fa1-95c2-fc4d1c8fde85', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 34');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (128, '2019-06-11 12:00:40.300283', 'e53c6cb0-b3dc-4f61-9d0f-6e730a040acd', 1, 'contact_person_insert', true, 'В таблицу contact_person добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (129, '2019-06-11 12:00:51.001895', '4b316648-1361-453b-b84f-7426dc9d9ba3', 1, 'contact_person_insert', true, 'В таблицу contact_person добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (130, '2019-06-11 12:03:10.169855', 'eee42395-18d4-4815-a344-24fbdf802069', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -10');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (131, '2019-06-11 12:03:10.172855', '5eb55d3b-daad-4318-b06c-99a0aa73a19d', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -9');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (132, '2019-06-11 12:03:10.177855', 'f24d9122-8656-4d44-82f4-86b814469888', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -12');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (133, '2019-06-11 12:03:10.187856', '37e034ee-72b6-4474-927a-8cd3049db467', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -13');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (134, '2019-06-11 12:03:10.188856', '6044799e-6196-4894-aed4-2683f81705e7', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -11');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (135, '2019-06-11 12:03:10.188856', '32ba7743-07c6-42b4-b05b-cb34882a9fe6', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (136, '2019-06-11 12:03:10.286862', 'f3481726-a68e-41fa-807e-72331d52af1f', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -8');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (137, '2019-06-11 12:03:10.309863', 'a1896934-8d9f-4585-8bf8-b55a71e21179', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -7');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (138, '2019-06-11 12:03:10.317864', 'de9e5d4e-f567-44b2-9d09-5d5a06e0b8f2', 1, 'contact_person_insert', true, 'В таблицу contact_person добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (139, '2019-06-11 12:03:34.278234', '439b78e0-35f9-42e2-bcae-d7a76daa53ed', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -15');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (140, '2019-06-11 12:03:34.284234', '5cf1223e-8a29-4764-8ee9-20b0a33ed82f', 1, 'contact_person_insert', true, 'В таблицу contact_person добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (141, '2019-06-11 12:04:35.510736', '917634d2-0cd9-4d61-a682-e9f2af69b09c', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором 5');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (142, '2019-06-11 12:04:37.349841', 'f7af39fd-a467-4263-9e83-ab995854aa46', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором 4');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (143, '2019-06-11 12:04:39.508965', '9a2b08e4-d8d2-407b-9f82-3b9014d31cf0', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором 3');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (144, '2019-06-11 12:04:41.365071', '3c311257-5bfb-4db3-9218-b13c3613d7f2', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором 2');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (145, '2019-06-11 12:05:01.363215', '05df4e36-fa55-49ac-b55f-b3030b5df445', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -25');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (146, '2019-06-11 12:05:01.370215', '874fb1d0-d42d-49e7-973b-7878211a3e38', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -27');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (147, '2019-06-11 12:05:01.374216', 'f7403693-7966-4200-be0b-a1a627c32287', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -28');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (148, '2019-06-11 12:05:01.393217', '51600599-28a8-4280-8e94-5865cb76295e', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -30');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (149, '2019-06-11 12:05:01.401217', '0c47ca37-8ed7-4f90-82ab-2a615cbce2f7', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -29');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (150, '2019-06-11 12:05:01.428219', '48aec86f-e8c9-42cd-8bf3-daf489ead17e', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -24');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (151, '2019-06-11 12:05:01.468221', '5b9ffa35-43e6-4176-afae-e60a4c95ed1a', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -26');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (152, '2019-06-11 12:05:01.494222', '1622bd72-a64d-420f-b52c-caf57de19dd5', 1, 'contact_person_insert', true, 'В таблицу contact_person добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (153, '2019-06-11 12:07:14.546833', 'a309e1d4-ab58-42a2-bb11-5ed7ce19fc4b', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -43');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (154, '2019-06-11 12:07:14.548833', 'af10e938-eed6-4534-9997-69b1612eabde', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -32');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (155, '2019-06-11 12:07:14.550833', '5f8c651e-7be2-411d-b25a-cd89333cb86b', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -45');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (156, '2019-06-11 12:07:14.551833', '1c1155ae-da19-4493-8878-7ca66ac1f59a', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -44');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (157, '2019-06-11 12:07:14.554833', 'e11d2c4f-6703-4a63-9197-811cb7cf337b', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -34');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (158, '2019-06-11 12:07:14.576834', '57a44ff2-71ac-435f-a320-667c2d13133f', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -33');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (159, '2019-06-11 12:07:14.718842', '6e5224a3-22f8-49a7-b478-18b6f6148e3f', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -35');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (160, '2019-06-11 12:07:14.719842', 'e2ece0d2-6ccc-443b-a851-80b439bd309b', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -36');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (161, '2019-06-11 12:07:14.733843', '03227689-ee8b-4a96-bcb0-a053666ab2a8', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -38');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (162, '2019-06-11 12:07:14.736843', 'efea4ee4-9799-471c-afe7-51dee53ce54c', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -39');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (163, '2019-06-11 12:07:14.743844', '14757f9e-8787-4c83-a785-0516b4c94e23', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -37');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (164, '2019-06-11 12:07:14.762845', '2f119da2-f421-40dc-9337-9265296ee265', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -40');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (165, '2019-06-11 12:07:14.840849', '7490427a-587f-4fff-a777-344c0cfe96d0', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -42');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (166, '2019-06-11 12:07:14.85385', '76014c59-a46b-4307-b11a-0614fc296ece', 1, 'contact_person_delete', true, 'Из таблицы contact_person удалена запись c идентификатором -41');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (167, '2019-06-11 12:07:14.883852', '4db190f4-b4f7-45f5-91cf-5087cb94cf98', 1, 'contact_person_insert', true, 'В таблицу contact_person добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (168, '2019-06-11 12:07:14.883852', 'ca0ba699-8161-48ad-838b-141ab3e208f2', 1, 'contact_person_insert', true, 'В таблицу contact_person добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (169, '2019-06-11 12:18:02.919915', '1eaca7de-32f2-4852-ae30-c565d1367292', 1, 'contact_person_update', true, 'В таблице contact_person отредактирована запись с идентификатором 7');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (170, '2019-06-11 12:18:05.500063', 'b9177fd8-f019-438c-a640-eaf345ca252e', 1, 'contact_person_update', true, 'В таблице contact_person отредактирована запись с идентификатором 7');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (171, '2019-06-11 23:13:37.479559', '04b9e7e6-c9d6-4b0e-9108-38bc63369d16', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (172, '2019-06-11 23:13:47.180114', '20c68783-0cd4-466a-b714-aeb46795679a', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (173, '2019-06-12 18:37:26.306437', 'c7d3014a-5e2b-445b-8d7c-8de713bb7845', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 34');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (174, '2019-06-12 18:37:49.866785', 'c6da69bc-f8a3-47ae-b9e0-069091f9fbf3', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (175, '2019-06-12 19:14:28.658549', 'da60fb27-3e66-4376-aedf-64df9b6a3be2', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (176, '2019-06-12 19:14:44.957481', '4e9aae40-eb43-41ad-92f0-62b8943768ad', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (177, '2019-06-12 19:14:55.354075', '15d58180-0be1-41e7-940d-50f812141d44', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (178, '2019-06-12 19:20:25.87198', '3796d3b8-51b8-43c2-9b01-539ada9c09c2', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (179, '2019-06-12 19:21:25.392384', '3df437c7-f1d7-472a-be16-60ae4b81bb33', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (180, '2019-06-12 20:58:54.452864', 'a583c8b2-e41c-4ad9-b702-9b03cdfe985e', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 5');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (181, '2019-06-12 21:04:06.513713', '8a14a06c-3b38-4330-8ee3-090251fbfe3b', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 5');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (182, '2019-06-13 09:26:19.452095', 'acda8962-6b10-4950-9920-375c5a8803a9', 1, 'case_insert', true, 'В таблицу case добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (183, '2019-06-13 09:27:06.612744', 'abb635ad-ce88-42a7-aba4-206dbc67af57', 1, 'case_delete', true, 'Из таблицы case удалена запись c идентификатором 21');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (184, '2019-06-13 09:29:25.919537', '8a1a7812-8702-4eb8-ac28-8a7a3972f8bb', 1, 'case_insert', true, 'В таблицу case добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (185, '2019-06-13 10:05:04.831834', '49a57bed-a87d-411f-82cb-98381e36e621', 1, 'get_achievement_group_structure', true, 'Иерархическое содержание группы достижений');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (186, '2019-06-13 10:55:31.553911', '991f9402-d175-4c0b-8ba2-4365d595822c', 1, 'learning_group_insert', true, 'В таблицу learning_group добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (187, '2019-06-13 10:55:37.806269', '79a7435d-dd7b-42b8-bf30-1ac68df24abd', 1, 'learning_group_update', true, 'В таблице learning_group отредактирована запись с идентификатором 7');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (188, '2019-06-13 11:37:51.754184', 'af1a794d-f37d-439e-b464-307bc2655aae', 1, 'sysuser_insert', true, 'В таблицу sysuser добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (189, '2019-06-13 11:38:33.043546', 'fab9ab76-f3b5-432a-9a80-0818173925fe', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 15');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (190, '2019-06-13 11:38:36.582748', 'c1129bd2-a9f1-4ed4-9766-079c1859cde2', 1, 'sysuser_delete', true, 'Из таблицы sysuser удалена запись c идентификатором 15');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (191, '2019-06-13 11:38:45.764274', '52ea6044-7eaa-4598-814c-bc2722e9e4fd', 1, 'case_delete', true, 'Из таблицы case удалена запись c идентификатором 22');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (192, '2019-06-13 11:39:02.779247', 'f6cff371-5d1d-4cb7-b150-9ce400813768', 1, 'case_insert', true, 'В таблицу case добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (193, '2019-06-13 11:39:11.181727', 'e3fdf23a-bf90-4f3b-97c0-0c1a5c9dfffd', 1, 'case_unit_insert', true, 'В таблицу case_unit добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (194, '2019-06-13 11:39:15.811992', '0bbb9506-28d5-43c5-a77a-71d638db469e', 1, 'case_unit_update', true, 'В таблице case_unit отредактирована запись с идентификатором 56');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (195, '2019-06-13 11:39:18.39414', 'a4cfd944-192c-4773-80a9-34ab9430ffd8', 1, 'task_insert', true, 'В таблицу task добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (196, '2019-06-13 11:39:21.632325', '25d4da21-cea6-4476-90aa-3f492223aba2', 1, 'task_update', true, 'В таблице task отредактирована запись с идентификатором 42');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (197, '2019-06-13 11:39:36.631183', '38385d64-f00f-4cb0-9ea5-3d7493ce90a3', 1, 'case_update', true, 'В таблице case отредактирована запись с идентификатором 23');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (198, '2019-06-13 11:39:36.635183', '20d05a0f-71d7-4764-aa36-30ad6bd77616', 1, 'case_update', true, 'В таблице case отредактирована запись с идентификатором 23');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (199, '2019-06-13 11:39:43.995604', 'e708b84a-beed-482b-a418-37a7e161e7c5', 1, 'case_delete', true, 'Из таблицы case удалена запись c идентификатором 23');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (200, '2019-06-13 11:39:52.581095', '98d9fe0d-09df-4adf-8f8c-c915e15f55bd', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 34');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (201, '2019-06-13 11:40:08.705018', '88ff892e-84c2-4b54-ad48-bb741faacda0', 1, 'quantum_insert', true, 'В таблицу quantum добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (202, '2019-06-13 11:40:21.119728', 'd30576e0-3816-4770-a059-ac702112b62a', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 35');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (203, '2019-06-13 11:40:35.433546', '5920d6b1-8007-4e0b-9ea7-c0a39a37debb', 1, 'quantum_delete', true, 'Из таблицы quantum удалена запись c идентификатором 35');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (204, '2019-06-13 11:40:45.191104', '57733338-9677-4a29-8968-066d0487a0d8', 1, 'learning_group_delete', true, 'Из таблицы learning_group удалена запись c идентификатором 7');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (205, '2019-06-13 11:41:02.272081', '661abbfd-d7ca-4de1-8a2e-3c51c7bc068d', 1, 'learning_group_insert', true, 'В таблицу learning_group добавлена новая запись');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (206, '2019-06-13 11:41:58.71731', 'fa703c62-ffc3-4194-be91-6bee42e4863d', 1, 'learning_group_hours_batch_submit', true, 'OK');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (207, '2019-06-13 11:42:01.591474', 'cf22fff9-6251-4133-b9a3-a240b20d266e', 1, 'learning_group_hours_batch_submit', true, 'OK');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (208, '2019-06-13 11:42:04.200624', 'cad4457c-8c4d-4655-8149-b02b044e528a', 1, 'learning_group_hours_batch_submit', true, 'OK');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (209, '2019-06-13 11:43:07.995272', 'a7cec7f2-bf56-4550-bcbc-447aea2e41ff', 1, 'learning_group_hours_batch_submit', true, 'OK');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (210, '2019-06-13 11:43:12.50653', '95a51572-6c4e-41be-9836-4a1a5d31c4df', 1, 'learning_group_hours_batch_submit', true, 'OK');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (211, '2019-06-13 11:43:23.983187', 'c844c395-e04b-4ffb-ad11-5af46ca7f816', 1, 'learning_group_hours_batch_submit', true, 'OK');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (212, '2019-06-13 11:43:45.54742', '973449bd-23ec-45ca-8a67-89e6293c6ff1', 1, 'learning_group_update', true, 'В таблице learning_group отредактирована запись с идентификатором 8');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (213, '2019-06-13 11:43:56.324037', 'fa8cef67-1579-4faa-992d-0769cd6218d8', 1, 'learning_group_delete', true, 'Из таблицы learning_group удалена запись c идентификатором 8');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (214, '2019-06-13 13:07:04.912985', 'f779b94a-c8fa-4fb9-93d9-ccb6ce49312f', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (215, '2019-06-13 13:07:15.197573', 'dbf437bb-0f15-4481-aced-d84116b02563', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (216, '2019-06-13 13:08:16.747093', '918f6389-3648-4051-b31d-e4693104bbc7', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (217, '2019-06-13 13:08:46.851815', '1a6d06a8-fe99-463f-a803-c730e77080b9', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (218, '2019-06-13 13:12:05.909201', '20672554-28e4-4eca-b9f6-74a97eb07f38', 1, 'sysuser_update', true, 'В таблице sysuser отредактирована запись с идентификатором 14');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (219, '2019-06-13 13:16:39.961876', 'a10dad80-d6a0-4223-a4fe-afe135ffce8c', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 6');
INSERT INTO public.action_log (id, dt, request_uid, user_id, action, success, description) VALUES (220, '2019-06-13 13:17:16.500966', '2a7c89ab-44c6-4e94-9323-8162bd69080a', 1, 'quantum_update', true, 'В таблице quantum отредактирована запись с идентификатором 6');


--
-- Data for Name: case; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public."case" (id, title, icon, description, passing_score, min_age, author_id, max_score, unit_count, status, task_count) VALUES (1, 'Кейс 1', 'def_icon', 'Чумовой кейс по получению первой ачивки', 100, 10, 4, 135, 1, 0, 1);
INSERT INTO public."case" (id, title, icon, description, passing_score, min_age, author_id, max_score, unit_count, status, task_count) VALUES (18, 'qwerty', 'blank', 'Описание', 1, 7, 1, 0, 1, 0, 0);
INSERT INTO public."case" (id, title, icon, description, passing_score, min_age, author_id, max_score, unit_count, status, task_count) VALUES (20, 'Кейс 2', 'blank', 'Описание кейса 2', 6, 9, 1, 0, 0, 0, 0);


--
-- Data for Name: case_achievement; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.case_achievement (id, case_id, achievement_id) VALUES (13, 1, 2);


--
-- Data for Name: case_required_achievement; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.case_required_achievement (id, case_id, achievement_id) VALUES (1, 1, 1);
INSERT INTO public.case_required_achievement (id, case_id, achievement_id) VALUES (12, 1, 6);
INSERT INTO public.case_required_achievement (id, case_id, achievement_id) VALUES (13, 1, 3);


--
-- Data for Name: case_unit; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.case_unit (id, title, sort_id, description, case_id) VALUES (1, 'Урок 1', 1, 'описание', 1);
INSERT INTO public.case_unit (id, title, sort_id, description, case_id) VALUES (55, 'Занятие 1', 1, 'wrwerwer', 18);


--
-- Data for Name: city; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.city (id, name, region_id) VALUES (2, 'Москва', 77);
INSERT INTO public.city (id, name, region_id) VALUES (4, 'Хабаровск', 27);
INSERT INTO public.city (id, name, region_id) VALUES (3, 'Нижний новгород', 52);
INSERT INTO public.city (id, name, region_id) VALUES (1, 'Самара', 63);
INSERT INTO public.city (id, name, region_id) VALUES (6, 'Тольятти', 63);
INSERT INTO public.city (id, name, region_id) VALUES (7, 'Новокуйбышевск', 63);


--
-- Data for Name: contact_person; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.contact_person (id, fullname, phone, relation, user_id) VALUES (1, 'Сидоров', '222-33-44', 'дядя', 3);
INSERT INTO public.contact_person (id, fullname, phone, relation, user_id) VALUES (20, 'aaa', 'ccc', 'bbb', 5);
INSERT INTO public.contact_person (id, fullname, phone, relation, user_id) VALUES (21, 'ddd', 'fff', 'eee', 5);
INSERT INTO public.contact_person (id, fullname, phone, relation, user_id) VALUES (22, 'aaa', 'xxxx', 'qqqq', 5);


--
-- Data for Name: date_substitution; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.date_substitution (id, target_date, substitute_date, city_id, timing_grid_id) VALUES (23, '2018-09-05', NULL, 1, NULL);
INSERT INTO public.date_substitution (id, target_date, substitute_date, city_id, timing_grid_id) VALUES (17, '2018-09-10', NULL, 1, NULL);
INSERT INTO public.date_substitution (id, target_date, substitute_date, city_id, timing_grid_id) VALUES (24, '2018-09-07', NULL, 1, 4);


--
-- Data for Name: learning_group; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.learning_group (id, title, quantum_id, teacher_id, substitute_teacher_id, min_students, max_students, start_date, finish_date, min_age, optional, timing_grid_id) VALUES (1, 'Тестовая группа', 1, 4, 4, 4, 10, '2018-08-19', '2000-01-01', 10, false, NULL);
INSERT INTO public.learning_group (id, title, quantum_id, teacher_id, substitute_teacher_id, min_students, max_students, start_date, finish_date, min_age, optional, timing_grid_id) VALUES (4, 'AAAAA', 1, 4, 4, 0, 20, '2019-05-28', '2019-08-26', 6, false, NULL);


--
-- Data for Name: learning_group_case; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.learning_group_case (id, learning_group_id, case_id, state) VALUES (1, 1, 1, 1);


--
-- Data for Name: learning_group_hours; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.learning_group_hours (id, day_of_week, order_num, learning_group_id) VALUES (1, 1, 2, NULL);
INSERT INTO public.learning_group_hours (id, day_of_week, order_num, learning_group_id) VALUES (2, 1, 3, NULL);
INSERT INTO public.learning_group_hours (id, day_of_week, order_num, learning_group_id) VALUES (3, 5, 6, NULL);
INSERT INTO public.learning_group_hours (id, day_of_week, order_num, learning_group_id) VALUES (4, 5, 7, NULL);


--
-- Data for Name: lesson; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.lesson (id, learning_group_id, lesson_date, lesson_time, held, teacher_id) VALUES (2, 1, '2019-03-03', '10:00:00', true, 3);


--
-- Data for Name: notice; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.notice (id, code, dt, receiver_id, description, viewed, sender_id) VALUES (1, 2, '2018-08-11 22:48:08.310679', 1, 'Произошла ошибка авторизации при логине пользователя', false, NULL);
INSERT INTO public.notice (id, code, dt, receiver_id, description, viewed, sender_id) VALUES (2, 2, '2018-08-11 22:48:14.001005', 1, 'Произошла ошибка авторизации при логине пользователя', false, NULL);
INSERT INTO public.notice (id, code, dt, receiver_id, description, viewed, sender_id) VALUES (3, 2, '2018-09-08 15:23:57.42632', 1, 'Произошла ошибка авторизации при логине пользователя', false, NULL);
INSERT INTO public.notice (id, code, dt, receiver_id, description, viewed, sender_id) VALUES (4, 2, '2018-10-01 20:53:26.104101', 1, 'Произошла ошибка авторизации при логине пользователя', false, NULL);


--
-- Data for Name: notice_type; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.notice_type (id, pattern, class, title) VALUES (1, 'пустой шаблон', 'UserMessageNotification', 'Сообщение пользователя');
INSERT INTO public.notice_type (id, pattern, class, title) VALUES (2, 'Произошла ошибка авторизации при логине пользователя', 'SimpleNotification', 'Неудачная попытка авторизации');
INSERT INTO public.notice_type (id, pattern, class, title) VALUES (3, 'Напоминаем о том, что завтра у Вас занятие в группе #lesson_course_name# (преподаватель #teacher_name#)', 'SimpleNotification', 'Напоминание о занятии');


--
-- Data for Name: quantum; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.quantum (id, name, icon, city_id, description) VALUES (1, 'Аэроквантум (Самара)', 'aero', 1, 'Обучение основам авиоконструкторства');
INSERT INTO public.quantum (id, name, icon, city_id, description) VALUES (2, 'Робоквантум (Самара)', 'robo', 1, 'Обучение основам машинного обучения и робототехники');
INSERT INTO public.quantum (id, name, icon, city_id, description) VALUES (3, 'IT-квантум (Самара)', 'it', 1, 'Основы программирования и компьютерной граммотности');
INSERT INTO public.quantum (id, name, icon, city_id, description) VALUES (4, 'Аэроквантум (Москва)', 'aero', 2, 'Обучение основам авиоконструкторства');
INSERT INTO public.quantum (id, name, icon, city_id, description) VALUES (5, 'Энерджиквантум (Нижний)', 'energy', 3, 'Обучение основам энергетики');
INSERT INTO public.quantum (id, name, icon, city_id, description) VALUES (6, 'Геоквантум (Хабаровск)', 'geo', 4, 'проверка');


--
-- Data for Name: region; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.region (id, name, timezone) VALUES (1, 'Республика Адыгея (Адыгея)', 3);
INSERT INTO public.region (id, name, timezone) VALUES (5, 'Республика Дагестан', 3);
INSERT INTO public.region (id, name, timezone) VALUES (6, 'Республика Ингушетия', 3);
INSERT INTO public.region (id, name, timezone) VALUES (7, 'Кабардино-Балкарская Республика', 3);
INSERT INTO public.region (id, name, timezone) VALUES (8, 'Республика Калмыкия', 3);
INSERT INTO public.region (id, name, timezone) VALUES (9, 'Карачаево-Черкесская Республика', 3);
INSERT INTO public.region (id, name, timezone) VALUES (10, 'Республика Карелия', 3);
INSERT INTO public.region (id, name, timezone) VALUES (11, 'Республика Коми', 3);
INSERT INTO public.region (id, name, timezone) VALUES (12, 'Республика Марий Эл', 3);
INSERT INTO public.region (id, name, timezone) VALUES (13, 'Республика Мордовия', 3);
INSERT INTO public.region (id, name, timezone) VALUES (15, 'Республика Северная Осетия - Алания', 3);
INSERT INTO public.region (id, name, timezone) VALUES (16, 'Республика Татарстан (Татарстан)', 3);
INSERT INTO public.region (id, name, timezone) VALUES (20, 'Чеченская Республика', 3);
INSERT INTO public.region (id, name, timezone) VALUES (21, 'Чувашская Республика - Чувашия', 3);
INSERT INTO public.region (id, name, timezone) VALUES (23, 'Краснодарский край', 3);
INSERT INTO public.region (id, name, timezone) VALUES (26, 'Ставропольский край', 3);
INSERT INTO public.region (id, name, timezone) VALUES (29, 'Архангельская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (31, 'Белгородская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (32, 'Брянская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (33, 'Владимирская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (34, 'Волгоградская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (35, 'Вологодская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (36, 'Воронежская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (37, 'Ивановская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (39, 'Калининградская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (40, 'Калужская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (43, 'Кировская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (44, 'Костромская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (46, 'Курская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (47, 'Ленинградская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (48, 'Липецкая область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (50, 'Московская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (51, 'Мурманская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (52, 'Нижегородская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (53, 'Новгородская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (57, 'Орловская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (58, 'Пензенская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (60, 'Псковская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (61, 'Ростовская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (62, 'Рязанская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (64, 'Саратовская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (67, 'Смоленская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (68, 'Тамбовская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (69, 'Тверская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (71, 'Тульская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (76, 'Ярославская область', 3);
INSERT INTO public.region (id, name, timezone) VALUES (77, 'г. Москва', 3);
INSERT INTO public.region (id, name, timezone) VALUES (78, 'Санкт-Петербург', 3);
INSERT INTO public.region (id, name, timezone) VALUES (83, 'Ненецкий автономный округ', 3);
INSERT INTO public.region (id, name, timezone) VALUES (91, 'Республика Крым', 3);
INSERT INTO public.region (id, name, timezone) VALUES (92, 'Севастополь', 3);
INSERT INTO public.region (id, name, timezone) VALUES (99, 'Иные территории, включая город и космодром Байконур', 3);
INSERT INTO public.region (id, name, timezone) VALUES (18, 'Удмуртская Республика', 4);
INSERT INTO public.region (id, name, timezone) VALUES (30, 'Астраханская область', 4);
INSERT INTO public.region (id, name, timezone) VALUES (63, 'Самарская область', 4);
INSERT INTO public.region (id, name, timezone) VALUES (73, 'Ульяновская область', 4);
INSERT INTO public.region (id, name, timezone) VALUES (55, 'Омская область', 6);
INSERT INTO public.region (id, name, timezone) VALUES (45, 'Курганская область', 5);
INSERT INTO public.region (id, name, timezone) VALUES (56, 'Оренбургская область', 5);
INSERT INTO public.region (id, name, timezone) VALUES (59, 'Пермский край', 5);
INSERT INTO public.region (id, name, timezone) VALUES (66, 'Свердловская область', 5);
INSERT INTO public.region (id, name, timezone) VALUES (72, 'Тюменская область', 5);
INSERT INTO public.region (id, name, timezone) VALUES (74, 'Челябинская область', 5);
INSERT INTO public.region (id, name, timezone) VALUES (86, 'Ханты-Мансийский автономный округ - Югра', 5);
INSERT INTO public.region (id, name, timezone) VALUES (89, 'Ямало-Ненецкий автономный округ', 5);
INSERT INTO public.region (id, name, timezone) VALUES (2, 'Республика Башкортостан', 5);
INSERT INTO public.region (id, name, timezone) VALUES (4, 'Республика Алтай', 7);
INSERT INTO public.region (id, name, timezone) VALUES (19, 'Республика Хакасия', 7);
INSERT INTO public.region (id, name, timezone) VALUES (17, 'Республика Тыва', 7);
INSERT INTO public.region (id, name, timezone) VALUES (54, 'Новосибирская область', 6);
INSERT INTO public.region (id, name, timezone) VALUES (70, 'Томская область', 6);
INSERT INTO public.region (id, name, timezone) VALUES (24, 'Красноярский край', 7);
INSERT INTO public.region (id, name, timezone) VALUES (22, 'Алтайский край', 7);
INSERT INTO public.region (id, name, timezone) VALUES (42, 'Кемеровская область', 7);
INSERT INTO public.region (id, name, timezone) VALUES (38, 'Иркутская область', 8);
INSERT INTO public.region (id, name, timezone) VALUES (3, 'Республика Бурятия', 8);
INSERT INTO public.region (id, name, timezone) VALUES (28, 'Амурская область', 9);
INSERT INTO public.region (id, name, timezone) VALUES (14, 'Республика Саха (Якутия)', 9);
INSERT INTO public.region (id, name, timezone) VALUES (75, 'Забайкальский край', 9);
INSERT INTO public.region (id, name, timezone) VALUES (79, 'Еврейская автономная область', 10);
INSERT INTO public.region (id, name, timezone) VALUES (25, 'Приморский край', 10);
INSERT INTO public.region (id, name, timezone) VALUES (27, 'Хабаровский край', 10);
INSERT INTO public.region (id, name, timezone) VALUES (49, 'Магаданская область', 10);
INSERT INTO public.region (id, name, timezone) VALUES (65, 'Сахалинская область', 11);
INSERT INTO public.region (id, name, timezone) VALUES (87, 'Чукотский автономный округ', 11);
INSERT INTO public.region (id, name, timezone) VALUES (41, 'Камчатский край', 12);
INSERT INTO public.region (id, name, timezone) VALUES (1000, 'Test region', 4);


--
-- Data for Name: report; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.report (id, city_id, title, template_url, sql_template, form_url) VALUES (1, 1, 'Календарно-тематический план', 'calendar_thematic_plan.rtf', 'select s.name || ''  ('' || s.description || '')'' as "name", s.unit_count  from course_skill cs
left join skill s on cs.skill_id=s.id
where course_id=#courseId#
order by cs.skill_group_id, cs,sort_id', 'forms/CalendarThematicPlan.qml');
INSERT INTO public.report (id, city_id, title, template_url, sql_template, form_url) VALUES (2, 1, 'Пользователи (демо)', 'users.rtf', 'select * from sysuser where sysuser_type_id=#sysuser_type_id#', 'forms/ReportUsers.qml');


--
-- Data for Name: sys_params; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.sys_params (id, db_version) VALUES (1, 2);


--
-- Data for Name: sysuser; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.sysuser (id, username, pswd, first_name, last_name, birth_date, gender, created, last_login, active, city_id, sysuser_type_id, second_name, speciality, phone, actual_address, passport_number, passport_series, passport_issue_date, passport_issue_authority, passport_address, passport_department_code, personnel_number, staffer, shift, balance, disable_reason, email, has_social_benefits, social_benefits_comment, document_type) VALUES (1, 'levolex', '7f0f01ef92a54610e4a433f68002cb5d', 'Лев', 'Алексеевский', '1981-09-15', true, '2017-11-10', '2019-06-19', true, 1, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, false, NULL, 0);
INSERT INTO public.sysuser (id, username, pswd, first_name, last_name, birth_date, gender, created, last_login, active, city_id, sysuser_type_id, second_name, speciality, phone, actual_address, passport_number, passport_series, passport_issue_date, passport_issue_authority, passport_address, passport_department_code, personnel_number, staffer, shift, balance, disable_reason, email, has_social_benefits, social_benefits_comment, document_type) VALUES (14, 'jjj', 'e1e7a7b851547e0eaa3230c582f1e4d0', 'levon', 'ccccccccc', '2000-01-06', false, '2019-05-30', NULL, true, 4, 2, 'bbbbbb', '', '+7(555)555-55-55', 'dfgdfg', '12345', '1234', '2000-01-20', 'aaaa', 'dfgdfg', '123-456', '', true, 1, 0, 'проверка', 'qqqq', false, '', 0);
INSERT INTO public.sysuser (id, username, pswd, first_name, last_name, birth_date, gender, created, last_login, active, city_id, sysuser_type_id, second_name, speciality, phone, actual_address, passport_number, passport_series, passport_issue_date, passport_issue_authority, passport_address, passport_department_code, personnel_number, staffer, shift, balance, disable_reason, email, has_social_benefits, social_benefits_comment, document_type) VALUES (12, 'aaaaa', 'aeb30f3fe514db31c618cc92f67b4ec2', 'zzzzzz', 'ccccc', '2000-01-26', true, '2019-05-25', NULL, true, 3, 2, 'bbbb', '', '+7()--', 'cccc', '123456', '1234', '2000-01-18', 'aaaa', 'bbbb', '123-456', '', true, 1, 0, '', '', false, '', 0);
INSERT INTO public.sysuser (id, username, pswd, first_name, last_name, birth_date, gender, created, last_login, active, city_id, sysuser_type_id, second_name, speciality, phone, actual_address, passport_number, passport_series, passport_issue_date, passport_issue_authority, passport_address, passport_department_code, personnel_number, staffer, shift, balance, disable_reason, email, has_social_benefits, social_benefits_comment, document_type) VALUES (4, 'bobylev', 'c32a96a8995daa7211e202d293af95a0', 'Сергей', 'Бобылев', '1967-03-05', true, '2017-11-12', '2019-06-08', true, 1, 3, 'Александрович', 'Робототехника', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, false, NULL, 0);
INSERT INTO public.sysuser (id, username, pswd, first_name, last_name, birth_date, gender, created, last_login, active, city_id, sysuser_type_id, second_name, speciality, phone, actual_address, passport_number, passport_series, passport_issue_date, passport_issue_authority, passport_address, passport_department_code, personnel_number, staffer, shift, balance, disable_reason, email, has_social_benefits, social_benefits_comment, document_type) VALUES (2, 'ivanov', '79706b383b26d8a29f1d6da55665a699', 'Иван', 'Иванов', '1981-09-15', true, '2017-11-10', '2019-06-11', true, 1, 1, 'Петрович', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, false, NULL, 0);
INSERT INTO public.sysuser (id, username, pswd, first_name, last_name, birth_date, gender, created, last_login, active, city_id, sysuser_type_id, second_name, speciality, phone, actual_address, passport_number, passport_series, passport_issue_date, passport_issue_authority, passport_address, passport_department_code, personnel_number, staffer, shift, balance, disable_reason, email, has_social_benefits, social_benefits_comment, document_type) VALUES (5, 'uvarova', '07d7b487b25826797a75e934ec4b64e5', 'Екатерина', 'Уварова', '2005-07-12', false, '2017-11-13', NULL, true, 1, 4, '', '', '+7()--', NULL, '12345', '12345', '2005-10-20', 'asdsad', 'asdsad', NULL, NULL, true, 1, 0, NULL, NULL, true, 'qwerty', 1);
INSERT INTO public.sysuser (id, username, pswd, first_name, last_name, birth_date, gender, created, last_login, active, city_id, sysuser_type_id, second_name, speciality, phone, actual_address, passport_number, passport_series, passport_issue_date, passport_issue_authority, passport_address, passport_department_code, personnel_number, staffer, shift, balance, disable_reason, email, has_social_benefits, social_benefits_comment, document_type) VALUES (3, 'qqq', '8749ed6666ec43c419595f6edf207a14', 'aaa', 'ccc', '2000-01-21', true, '2017-11-11', NULL, true, 4, 3, 'bbb', '', '+7(444)444-44-44', 'ghjgh', '123456', '1234', '2000-01-12', 'asdasd', 'asdasd', '321-654', '1234', true, 1, 4, 'aaaa', 'sdfsdf', false, NULL, 0);


--
-- Data for Name: sysuser_type; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.sysuser_type (id, name) VALUES (0, 'Суперпользователь');
INSERT INTO public.sysuser_type (id, name) VALUES (3, 'Преподаватель');
INSERT INTO public.sysuser_type (id, name) VALUES (4, 'Ученик');
INSERT INTO public.sysuser_type (id, name) VALUES (1, 'Администратор');
INSERT INTO public.sysuser_type (id, name) VALUES (2, 'Директор');


--
-- Data for Name: task; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.task (id, name, max_score, sort_id, description, case_id) VALUES (1, 'задание 1', 135, 1, 'выаываыв', 1);


--
-- Data for Name: teacher_quantum; Type: TABLE DATA; Schema: public; Owner: postgres
--



--
-- Data for Name: test; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.test (id, name, value) VALUES (1, 'Test 1', 1);
INSERT INTO public.test (id, name, value) VALUES (2, 'Test 2', 34);
INSERT INTO public.test (id, name, value) VALUES (12, 'qqqq', 44);
INSERT INTO public.test (id, name, value) VALUES (11, 'qqqq', 11);
INSERT INTO public.test (id, name, value) VALUES (13, 'zzz', 55);
INSERT INTO public.test (id, name, value) VALUES (14, 'hhh', 333);


--
-- Data for Name: timing_grid; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.timing_grid (id, name, city_id, duration, city_default, version) VALUES (6, 'Московское особое', 2, 40, false, 7);
INSERT INTO public.timing_grid (id, name, city_id, duration, city_default, version) VALUES (4, 'Супер', 1, 50, false, 5);
INSERT INTO public.timing_grid (id, name, city_id, duration, city_default, version) VALUES (10, 'По умолчанию', 3, 40, true, 11);
INSERT INTO public.timing_grid (id, name, city_id, duration, city_default, version) VALUES (11, 'По умолчанию', 4, 40, true, 12);
INSERT INTO public.timing_grid (id, name, city_id, duration, city_default, version) VALUES (12, 'По умолчанию', 6, 40, true, 13);
INSERT INTO public.timing_grid (id, name, city_id, duration, city_default, version) VALUES (13, 'По умолчанию', 7, 40, true, 14);
INSERT INTO public.timing_grid (id, name, city_id, duration, city_default, version) VALUES (1, 'Самара (по умолчанию)', 1, 40, true, 1);
INSERT INTO public.timing_grid (id, name, city_id, duration, city_default, version) VALUES (5, 'Москва (по умолчанию)', 2, 40, true, 6);
INSERT INTO public.timing_grid (id, name, city_id, duration, city_default, version) VALUES (18, 'Проверка', 1, 40, false, 6);


--
-- Data for Name: timing_grid_unit; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (24, 4, 1, 3, '03:00:00');
INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (1, 1, 0, 1, '08:30:00');
INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (2, 1, 0, 2, '09:20:00');
INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (18, 1, 3, 3, '10:30:00');
INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (3, 1, 0, 3, '10:00:00');
INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (4, 1, 0, 4, '11:20:00');
INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (5, 1, 0, 5, '12:20:00');
INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (6, 1, 0, 6, '13:15:00');
INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (11, 1, 2, 8, '19:00:00');
INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (7, 1, 0, 7, '14:10:00');
INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (8, 1, 0, 8, '15:05:00');
INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (25, 18, 3, 3, '03:00:00');
INSERT INTO public.timing_grid_unit (id, timing_grid_id, day_type, order_num, begin_time) VALUES (26, 18, 5, 4, '05:00:00');


--
-- Data for Name: user_achievement; Type: TABLE DATA; Schema: public; Owner: postgres
--



--
-- Data for Name: user_case; Type: TABLE DATA; Schema: public; Owner: postgres
--



--
-- Data for Name: user_learning_group; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.user_learning_group (id, user_id, learning_group_id) VALUES (1, 3, 1);


--
-- Data for Name: user_lesson; Type: TABLE DATA; Schema: public; Owner: postgres
--



--
-- Data for Name: user_task; Type: TABLE DATA; Schema: public; Owner: postgres
--



--
-- Name: achievement_group_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.achievement_group_id_seq', 7, true);


--
-- Name: achievement_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.achievement_id_seq', 7, true);


--
-- Name: action_log_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.action_log_id_seq', 220, true);


--
-- Name: case_achievement_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.case_achievement_id_seq', 13, true);


--
-- Name: case_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.case_id_seq', 23, true);


--
-- Name: case_required_achievement_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.case_required_achievement_id_seq', 13, true);


--
-- Name: case_unit_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.case_unit_id_seq', 56, true);


--
-- Name: child_achievement_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.child_achievement_id_seq', 31, true);


--
-- Name: city_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.city_id_seq', 11, true);


--
-- Name: contact_person_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.contact_person_id_seq', 22, true);


--
-- Name: date_substitution_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.date_substitution_id_seq', 24, true);


--
-- Name: learning_group_case_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.learning_group_case_id_seq', 1, true);


--
-- Name: learning_group_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.learning_group_id_seq', 8, true);


--
-- Name: lesson_course_hours_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.lesson_course_hours_id_seq', 8, true);


--
-- Name: lesson_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.lesson_id_seq', 2, true);


--
-- Name: notice_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.notice_id_seq', 4, true);


--
-- Name: quantum_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.quantum_id_seq', 35, true);


--
-- Name: region_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.region_id_seq', 100, true);


--
-- Name: report_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.report_id_seq', 2, true);


--
-- Name: structure_version_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.structure_version_seq', 7, true);


--
-- Name: task_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.task_id_seq', 42, true);


--
-- Name: teacher_quantum_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.teacher_quantum_id_seq', 3, true);


--
-- Name: test_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.test_id_seq', 14, true);


--
-- Name: timing_grid_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.timing_grid_id_seq', 18, true);


--
-- Name: timing_grid_unit_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.timing_grid_unit_id_seq', 26, true);


--
-- Name: timing_grid_ver_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.timing_grid_ver_seq', 6, true);


--
-- Name: user_achievement_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.user_achievement_id_seq', 43, true);


--
-- Name: user_case_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.user_case_id_seq', 26, true);


--
-- Name: user_learning_group_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.user_learning_group_id_seq', 7, true);


--
-- Name: user_lesson_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.user_lesson_id_seq', 18, true);


--
-- Name: user_task_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.user_task_id_seq', 22, true);


--
-- Name: users_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.users_id_seq', 15, true);


--
-- Name: date_substitution date_substitution_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.date_substitution
    ADD CONSTRAINT date_substitution_pk PRIMARY KEY (id);


--
-- Name: date_substitution date_substitution_uq; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.date_substitution
    ADD CONSTRAINT date_substitution_uq UNIQUE (target_date, city_id);


--
-- Name: achievement pk_achievement; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.achievement
    ADD CONSTRAINT pk_achievement PRIMARY KEY (id);


--
-- Name: achievement_group pk_achievement_group; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.achievement_group
    ADD CONSTRAINT pk_achievement_group PRIMARY KEY (id);


--
-- Name: action_log pk_action_log; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.action_log
    ADD CONSTRAINT pk_action_log PRIMARY KEY (id);


--
-- Name: case pk_case; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."case"
    ADD CONSTRAINT pk_case PRIMARY KEY (id);


--
-- Name: case_achievement pk_case_achivement; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_achievement
    ADD CONSTRAINT pk_case_achivement PRIMARY KEY (id);


--
-- Name: case_required_achievement pk_case_required_achievement; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_required_achievement
    ADD CONSTRAINT pk_case_required_achievement PRIMARY KEY (id);


--
-- Name: case_unit pk_case_unit; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_unit
    ADD CONSTRAINT pk_case_unit PRIMARY KEY (id);


--
-- Name: achievement_children pk_child_achievement; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.achievement_children
    ADD CONSTRAINT pk_child_achievement PRIMARY KEY (id);


--
-- Name: city pk_city; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.city
    ADD CONSTRAINT pk_city PRIMARY KEY (id);


--
-- Name: contact_person pk_contact_person; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.contact_person
    ADD CONSTRAINT pk_contact_person PRIMARY KEY (id);


--
-- Name: learning_group pk_learning_group; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.learning_group
    ADD CONSTRAINT pk_learning_group PRIMARY KEY (id);


--
-- Name: learning_group_case pk_learning_group_case; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.learning_group_case
    ADD CONSTRAINT pk_learning_group_case PRIMARY KEY (id);


--
-- Name: learning_group_hours pk_learning_group_hours; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.learning_group_hours
    ADD CONSTRAINT pk_learning_group_hours PRIMARY KEY (id);


--
-- Name: lesson pk_lesson; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.lesson
    ADD CONSTRAINT pk_lesson PRIMARY KEY (id);


--
-- Name: notice pk_notice; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.notice
    ADD CONSTRAINT pk_notice PRIMARY KEY (id);


--
-- Name: notice_type pk_notice_type; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.notice_type
    ADD CONSTRAINT pk_notice_type PRIMARY KEY (id);


--
-- Name: quantum pk_quantum; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.quantum
    ADD CONSTRAINT pk_quantum PRIMARY KEY (id);


--
-- Name: region pk_region; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.region
    ADD CONSTRAINT pk_region PRIMARY KEY (id);


--
-- Name: report pk_report; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.report
    ADD CONSTRAINT pk_report PRIMARY KEY (id);


--
-- Name: sys_params pk_sys_params; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.sys_params
    ADD CONSTRAINT pk_sys_params PRIMARY KEY (id);


--
-- Name: sysuser_type pk_sysuser_type; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.sysuser_type
    ADD CONSTRAINT pk_sysuser_type PRIMARY KEY (id);


--
-- Name: task pk_task; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.task
    ADD CONSTRAINT pk_task PRIMARY KEY (id);


--
-- Name: teacher_quantum pk_teacher_quantum; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.teacher_quantum
    ADD CONSTRAINT pk_teacher_quantum PRIMARY KEY (id);


--
-- Name: timing_grid pk_timing_grid; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.timing_grid
    ADD CONSTRAINT pk_timing_grid PRIMARY KEY (id);


--
-- Name: timing_grid_unit pk_timing_grid_unit; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.timing_grid_unit
    ADD CONSTRAINT pk_timing_grid_unit PRIMARY KEY (id);


--
-- Name: user_achievement pk_user_achievement; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_achievement
    ADD CONSTRAINT pk_user_achievement PRIMARY KEY (id);


--
-- Name: user_case pk_user_case; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_case
    ADD CONSTRAINT pk_user_case PRIMARY KEY (id);


--
-- Name: user_learning_group pk_user_learning_group; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_learning_group
    ADD CONSTRAINT pk_user_learning_group PRIMARY KEY (id);


--
-- Name: user_lesson pk_user_lesson; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_lesson
    ADD CONSTRAINT pk_user_lesson PRIMARY KEY (id);


--
-- Name: user_task pk_user_task; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_task
    ADD CONSTRAINT pk_user_task PRIMARY KEY (id);


--
-- Name: sysuser pk_users; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.sysuser
    ADD CONSTRAINT pk_users PRIMARY KEY (id);


--
-- Name: test test_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.test
    ADD CONSTRAINT test_pkey PRIMARY KEY (id);


--
-- Name: case_achievement uq_case_achievement; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_achievement
    ADD CONSTRAINT uq_case_achievement UNIQUE (case_id, achievement_id);


--
-- Name: case_required_achievement uq_case_required_achievement; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_required_achievement
    ADD CONSTRAINT uq_case_required_achievement UNIQUE (case_id, achievement_id);


--
-- Name: case_unit uq_case_unit; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_unit
    ADD CONSTRAINT uq_case_unit UNIQUE (case_id, sort_id);


--
-- Name: achievement_children uq_child_achievement; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.achievement_children
    ADD CONSTRAINT uq_child_achievement UNIQUE (achievement_id, child_id);


--
-- Name: user_case uq_user_case; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_case
    ADD CONSTRAINT uq_user_case UNIQUE (user_id, case_id);


--
-- Name: user_learning_group uq_user_learning_group; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_learning_group
    ADD CONSTRAINT uq_user_learning_group UNIQUE (user_id, learning_group_id);


--
-- Name: user_task uq_user_task; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_task
    ADD CONSTRAINT uq_user_task UNIQUE (user_id, task_id);


--
-- Name: achievement_children achievement_children_iu; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER achievement_children_iu AFTER INSERT OR UPDATE ON public.achievement_children FOR EACH ROW EXECUTE PROCEDURE public.child_achievement_check();


--
-- Name: case_achievement case_achievement_ins; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER case_achievement_ins AFTER INSERT ON public.case_achievement FOR EACH ROW EXECUTE PROCEDURE public.on_insert_case_achievements();


--
-- Name: case case_ins_upd_del; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER case_ins_upd_del BEFORE INSERT OR DELETE OR UPDATE ON public."case" FOR EACH ROW EXECUTE PROCEDURE public.case_modification_constraints();


--
-- Name: city on_city_insert; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER on_city_insert AFTER INSERT ON public.city FOR EACH ROW EXECUTE PROCEDURE public.insert_city_default_timing_grid();


--
-- Name: case_required_achievement on_insert_case_required_achievements; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER on_insert_case_required_achievements AFTER INSERT ON public.case_required_achievement FOR EACH ROW EXECUTE PROCEDURE public.on_insert_case_required_achievements();


--
-- Name: user_case tr_after_user_case; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER tr_after_user_case AFTER INSERT OR DELETE OR UPDATE ON public.user_case FOR EACH ROW EXECUTE PROCEDURE public.case_passed();


--
-- Name: case_unit update_case_on_case_unit_change; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER update_case_on_case_unit_change AFTER INSERT OR DELETE OR UPDATE ON public.case_unit FOR EACH ROW EXECUTE PROCEDURE public.update_case_after_case_unit();


--
-- Name: task update_case_unit_on_task_change; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER update_case_unit_on_task_change AFTER INSERT OR DELETE OR UPDATE ON public.task FOR EACH ROW EXECUTE PROCEDURE public.update_case_unit_after_task();


--
-- Name: TRIGGER update_case_unit_on_task_change ON task; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TRIGGER update_case_unit_on_task_change ON public.task IS 'Обновление полей урока при изменении заданий';


--
-- Name: user_achievement user_achiement_delete; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER user_achiement_delete AFTER DELETE ON public.user_achievement FOR EACH ROW EXECUTE PROCEDURE public.user_achievement_delete();


--
-- Name: user_achievement user_achievement_ad_dependants; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER user_achievement_ad_dependants AFTER DELETE ON public.user_achievement FOR EACH ROW EXECUTE PROCEDURE public.delete_achievement_dependants();


--
-- Name: user_achievement user_achievement_add_children; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER user_achievement_add_children BEFORE INSERT ON public.user_achievement FOR EACH ROW EXECUTE PROCEDURE public.user_achievement_set_children();


--
-- Name: user_achievement user_achievement_ai_reset_expire; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER user_achievement_ai_reset_expire AFTER INSERT ON public.user_achievement FOR EACH ROW EXECUTE PROCEDURE public.user_achievement_insert();


--
-- Name: user_achievement user_achievement_bi; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER user_achievement_bi BEFORE INSERT ON public.user_achievement FOR EACH ROW EXECUTE PROCEDURE public.user_achievement_insert_expire();


--
-- Name: date_substitution date_substitution_city_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.date_substitution
    ADD CONSTRAINT date_substitution_city_fk FOREIGN KEY (city_id) REFERENCES public.city(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: date_substitution date_substitution_timing_grid_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.date_substitution
    ADD CONSTRAINT date_substitution_timing_grid_fk FOREIGN KEY (timing_grid_id) REFERENCES public.timing_grid(id) ON UPDATE CASCADE ON DELETE SET NULL;


--
-- Name: achievement fk_achievement_achievement_group; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.achievement
    ADD CONSTRAINT fk_achievement_achievement_group FOREIGN KEY (achievement_group_id) REFERENCES public.achievement_group(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: achievement_group fk_achievement_group_city; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.achievement_group
    ADD CONSTRAINT fk_achievement_group_city FOREIGN KEY (city_id) REFERENCES public.city(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: achievement_group fk_achievement_group_parent; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.achievement_group
    ADD CONSTRAINT fk_achievement_group_parent FOREIGN KEY (parent_id) REFERENCES public.achievement_group(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: action_log fk_action_log_user; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.action_log
    ADD CONSTRAINT fk_action_log_user FOREIGN KEY (user_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE SET NULL;


--
-- Name: case_achievement fk_case_achievement_achivement; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_achievement
    ADD CONSTRAINT fk_case_achievement_achivement FOREIGN KEY (achievement_id) REFERENCES public.achievement(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: case_achievement fk_case_achievement_case; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_achievement
    ADD CONSTRAINT fk_case_achievement_case FOREIGN KEY (case_id) REFERENCES public."case"(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: case_required_achievement fk_case_required_achievement_achievement; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_required_achievement
    ADD CONSTRAINT fk_case_required_achievement_achievement FOREIGN KEY (achievement_id) REFERENCES public.achievement(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: case_required_achievement fk_case_required_achievement_case; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_required_achievement
    ADD CONSTRAINT fk_case_required_achievement_case FOREIGN KEY (case_id) REFERENCES public."case"(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: case_unit fk_case_unit_case; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.case_unit
    ADD CONSTRAINT fk_case_unit_case FOREIGN KEY (case_id) REFERENCES public."case"(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: case fk_case_user; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."case"
    ADD CONSTRAINT fk_case_user FOREIGN KEY (author_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: achievement_children fk_child_achievement_child; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.achievement_children
    ADD CONSTRAINT fk_child_achievement_child FOREIGN KEY (child_id) REFERENCES public.achievement(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: achievement_children fk_child_achievement_target; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.achievement_children
    ADD CONSTRAINT fk_child_achievement_target FOREIGN KEY (achievement_id) REFERENCES public.achievement(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: city fk_city_region; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.city
    ADD CONSTRAINT fk_city_region FOREIGN KEY (region_id) REFERENCES public.region(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: contact_person fk_contact_person_user; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.contact_person
    ADD CONSTRAINT fk_contact_person_user FOREIGN KEY (user_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: learning_group_case fk_learning_group_case_case; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.learning_group_case
    ADD CONSTRAINT fk_learning_group_case_case FOREIGN KEY (case_id) REFERENCES public."case"(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: learning_group_case fk_learning_group_case_group; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.learning_group_case
    ADD CONSTRAINT fk_learning_group_case_group FOREIGN KEY (learning_group_id) REFERENCES public.learning_group(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: learning_group_hours fk_learning_group_hours_group; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.learning_group_hours
    ADD CONSTRAINT fk_learning_group_hours_group FOREIGN KEY (learning_group_id) REFERENCES public.learning_group(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: learning_group fk_learning_group_quantum; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.learning_group
    ADD CONSTRAINT fk_learning_group_quantum FOREIGN KEY (quantum_id) REFERENCES public.quantum(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: learning_group fk_learning_group_substitute_teacher; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.learning_group
    ADD CONSTRAINT fk_learning_group_substitute_teacher FOREIGN KEY (substitute_teacher_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE SET NULL;


--
-- Name: learning_group fk_learning_group_user; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.learning_group
    ADD CONSTRAINT fk_learning_group_user FOREIGN KEY (teacher_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: lesson fk_lesson_learning_group; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.lesson
    ADD CONSTRAINT fk_lesson_learning_group FOREIGN KEY (learning_group_id) REFERENCES public.learning_group(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: lesson fk_lesson_teacher; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.lesson
    ADD CONSTRAINT fk_lesson_teacher FOREIGN KEY (teacher_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: notice fk_notice_receiver; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.notice
    ADD CONSTRAINT fk_notice_receiver FOREIGN KEY (receiver_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: notice fk_notice_sender; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.notice
    ADD CONSTRAINT fk_notice_sender FOREIGN KEY (sender_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: quantum fk_quantum_city; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.quantum
    ADD CONSTRAINT fk_quantum_city FOREIGN KEY (city_id) REFERENCES public.city(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: report fk_report_city; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.report
    ADD CONSTRAINT fk_report_city FOREIGN KEY (city_id) REFERENCES public.city(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: task fk_task_case; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.task
    ADD CONSTRAINT fk_task_case FOREIGN KEY (case_id) REFERENCES public."case"(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: teacher_quantum fk_teacher_quantum_quantum; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.teacher_quantum
    ADD CONSTRAINT fk_teacher_quantum_quantum FOREIGN KEY (quantum_id) REFERENCES public.quantum(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: teacher_quantum fk_teacher_quantum_user; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.teacher_quantum
    ADD CONSTRAINT fk_teacher_quantum_user FOREIGN KEY (teacher_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: timing_grid fk_timing_grid_city; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.timing_grid
    ADD CONSTRAINT fk_timing_grid_city FOREIGN KEY (city_id) REFERENCES public.city(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: timing_grid_unit fk_timing_grid_unit_grid; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.timing_grid_unit
    ADD CONSTRAINT fk_timing_grid_unit_grid FOREIGN KEY (timing_grid_id) REFERENCES public.timing_grid(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: user_achievement fk_user_achievement_achievement; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_achievement
    ADD CONSTRAINT fk_user_achievement_achievement FOREIGN KEY (achievement_id) REFERENCES public.achievement(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: user_achievement fk_user_achievement_user; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_achievement
    ADD CONSTRAINT fk_user_achievement_user FOREIGN KEY (user_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: user_case fk_user_case_case; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_case
    ADD CONSTRAINT fk_user_case_case FOREIGN KEY (case_id) REFERENCES public."case"(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: user_case fk_user_case_user; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_case
    ADD CONSTRAINT fk_user_case_user FOREIGN KEY (user_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: sysuser fk_user_city; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.sysuser
    ADD CONSTRAINT fk_user_city FOREIGN KEY (city_id) REFERENCES public.city(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: user_learning_group fk_user_learning_group_group; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_learning_group
    ADD CONSTRAINT fk_user_learning_group_group FOREIGN KEY (learning_group_id) REFERENCES public.learning_group(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: user_learning_group fk_user_learning_group_user; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_learning_group
    ADD CONSTRAINT fk_user_learning_group_user FOREIGN KEY (user_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: user_lesson fk_user_lesson_case; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_lesson
    ADD CONSTRAINT fk_user_lesson_case FOREIGN KEY (case_id) REFERENCES public."case"(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: user_lesson fk_user_lesson_lesson; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_lesson
    ADD CONSTRAINT fk_user_lesson_lesson FOREIGN KEY (lesson_id) REFERENCES public.lesson(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: user_lesson fk_user_lesson_user; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_lesson
    ADD CONSTRAINT fk_user_lesson_user FOREIGN KEY (user_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: user_task fk_user_task_task; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_task
    ADD CONSTRAINT fk_user_task_task FOREIGN KEY (task_id) REFERENCES public.task(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: user_task fk_user_task_user; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.user_task
    ADD CONSTRAINT fk_user_task_user FOREIGN KEY (user_id) REFERENCES public.sysuser(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: sysuser fk_user_user_type; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.sysuser
    ADD CONSTRAINT fk_user_user_type FOREIGN KEY (sysuser_type_id) REFERENCES public.sysuser_type(id) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- PostgreSQL database dump complete
--

