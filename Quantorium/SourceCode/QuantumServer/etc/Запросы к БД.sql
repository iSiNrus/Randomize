--Получение максимального балла по всем заданиям курса

SELECT c.id course_id, sum(t.max_score) FROM course c
LEFT JOIN skill_set ss on c.id=ss.course_id
LEFT JOIN skill s on ss.id=s.skill_set_id
LEFT JOIN lesson l on s.id=l.skill_id
LEFT JOIN task t on l.id=t.lesson_id
GROUP BY c.id

--Получение максимального балла по всем урокам курса
SELECT c.id course_id, sum(l.max_score) FROM course c
LEFT JOIN skill_set ss on c.id=ss.course_id
LEFT JOIN skill s on ss.id=s.skill_set_id
LEFT JOIN lesson l on s.id=l.skill_id
GROUP BY c.id

--Определение доступности навыка для ученика
SELECT count(rs.required_skill_id) = count(cs.skill_id)
FROM required_skill rs
FULL JOIN completed_skill cs on rs.required_skill_id=cs.skill_id and cs.user_id=:user_id
WHERE rs.skill_id=:skill_id

--Получение всех доступных для ученика навыков курса
SELECT s.* FROM skill s
LEFT JOIN skill_group sg on s.skill_group_id=sg.id
LEFT JOIN course c on sg.course_id=c.id
LEFT JOIN quantum q on c.quantum_id=q.id
WHERE 1=1 
{AND q.id=:quantum_id}
{AND c.id=:course_id}
{AND (
SELECT count(rs.required_skill_id) = count(cs.skill_id)
FROM required_skill rs
FULL JOIN completed_skill cs on rs.required_skill_id=cs.skill_id and cs.user_id=:user_id
WHERE rs.skill_id=s.id
)}

--Получить список групп (занятий) по указанному навыку
SELECT lc.*, 
(select count('x') from user_lesson_course where lesson_course_id=lc.id) students_count,
su.first_name, su.second_name, su.last_name, s.min_age FROM lesson_course lc
LEFT JOIN skill s on lc.skill_id=s.id
LEFT JOIN sysuser su on lc.teacher_id = su.id
WHERE lc.skill_id=:skill_id

--Получить сумму баллов по курсу 
--За занятия
select sum(cl.score)
from course_skill_group csg
inner join course_skill cs on csg.course_id=cs.course_id and csg.skill_group_id=cs.skill_group_id
inner join skill_unit su on cs.skill_id=su.skill_id
inner join completed_lesson cl on su.id=cl.lesson_id
where csg.course_id=:course_id
--За задания
select sum(ct.score)
from course_skill_group csg
inner join course_skill cs on csg.course_id=cs.course_id and csg.skill_group_id=cs.skill_group_id
inner join skill_unit su on cs.skill_id=su.skill_id
inner join task t on su.id=t.skill_unit_id
inner join completed_task ct on t.id=ct.task_id
where csg.course_id=:course_id


--Запрос данных курса относительно ученика
select c.id, 
(select count(rs.id)=count(cc.id) from required_course rs 
left join completed_course cc on rs.required_course_id = cc.course_id and cc.user_id=:user_id 
where rs.course_id=c.id) knowledge_condition, 
(select date_part('year', age('now', birth_date)) >= c.min_age from sysuser where id=:user_id) age_condition, 
exists(select 'x' from completed_course where user_id=:user_id and course_id=c.id) completed, 
(select sum(cl.score) 
from course_skill_group csg 
inner join course_skill cs on csg.course_id=cs.course_id and csg.skill_group_id=cs.skill_group_id 
inner join skill_unit su on cs.skill_id=su.skill_id 
inner join completed_lesson cl on su.id=cl.lesson_id 
where csg.course_id=c.id and cl.user_id=:user_id) lesson_score, 
(select sum(ct.score) 
from course_skill_group csg 
inner join course_skill cs on csg.course_id=cs.course_id and csg.skill_group_id=cs.skill_group_id 
inner join skill_unit su on cs.skill_id=su.skill_id 
inner join task t on su.id=t.skill_unit_id 
inner join completed_task ct on t.id=ct.task_id 
where csg.course_id=c.id and ct.user_id=:user_id) task_score 
from course c --where c.quantum_id=:quantum_id 


--Запрос на получение информации доступности навыков для конкретного ученика
select s.*, (cs.id is not null) completed,
exists(select 'x' from user_lesson_course where skill_id=s.skill_id and user_id=3) in_study,
exists(select 'x' from course_skill s1
inner join completed_skill c1 on s1.skill_id=c1.skill_id and c1.user_id=3
where s1.skill_group_id=s.skill_group_id and s1.sort_id=s.sort_id-1) previous_skill_completed,
(select count(s2.skill_id) = count(c2.skill_id)
from required_skill_group rsg
left join course_skill s2 on rsg.required_skill_group_id=s2.skill_group_id and s2.course_id=rsg.course_id
left join completed_skill c2 on s2.skill_id=c2.skill_id and c2.user_id=3
where rsg.course_id=1 and rsg.skill_group_id=s.skill_group_id) previous_group_completed
from course_skill s
left join completed_skill cs on s.skill_id=cs.skill_id and cs.user_id=3;


--Запрос на получение состава и иерархии групп навыков и навыков для различных курсов
SELECT csg.course_id, csg.skill_group_id, csg.required_skill_group_id, csgs.skill_id, csgs.required_skill_id 
FROM course_skill_group csg
LEFT JOIN course_skill_group_skill csgs on csg.course_id=csgs.course_id and csg.skill_group_id=csgs.skill_group_id
--WHERE csg.course_id = 1
ORDER BY csg.course_id, csg.skill_group_id

--Запрос на получение обобщенных данных об ученике
select u.first_name, u.last_name, u.gender, info.* 
from sysuser u 
left join 
(select ulc.user_id, count('x') skill_count, 
max((select sum(cl1.score) from completed_lesson cl1 
inner join skill_unit su1 on cl1.skill_unit_id=su1.id 
where cl1.user_id=ulc.user_id and su1.skill_id=ulc.skill_id)) lesson_score, 
max((select sum(ct.score) from completed_task ct 
left join task t on ct.task_id=t.id 
left join skill_unit su2 on t.skill_unit_id=su2.id 
where ct.user_id=ulc.user_id and su2.skill_id=ulc.skill_id)) task_score 
from user_lesson_course ulc 
group by ulc.user_id) as info on u.id=info.user_id 
where u.id=%1

--Запрос получающий время начала первого урока и время окончания последнего
SELECT lch.lesson_course_id AS id, lch.lesson_course_id,
min(
	CASE
		WHEN lch.day_of_week = 1 THEN COALESCE(tgu2.begin_time, tgu.begin_time)
		ELSE NULL::time without time zone
	END) AS monday_begin_time,
max(
	CASE
		WHEN lch.day_of_week = 1 THEN COALESCE(tgu2.begin_time, tgu.begin_time) + '00:40:00'::interval
		ELSE NULL::time without time zone
	END) AS monday_end_time,
min(
	CASE
		WHEN lch.day_of_week = 2 THEN COALESCE(tgu2.begin_time, tgu.begin_time)
		ELSE NULL::time without time zone
	END) AS tuesday_begin_time,
max(
	CASE
		WHEN lch.day_of_week = 2 THEN COALESCE(tgu2.begin_time, tgu.begin_time) + '00:40:00'::interval
		ELSE NULL::time without time zone
	END) AS tuesday_end_time,
min(
	CASE
		WHEN lch.day_of_week = 3 THEN COALESCE(tgu2.begin_time, tgu.begin_time)
		ELSE NULL::time without time zone
	END) AS wednesday_begin_time,
max(
	CASE
		WHEN lch.day_of_week = 3 THEN COALESCE(tgu2.begin_time, tgu.begin_time) + '00:40:00'::interval
		ELSE NULL::time without time zone
	END) AS wednesday_end_time,
min(
	CASE
		WHEN lch.day_of_week = 4 THEN COALESCE(tgu2.begin_time, tgu.begin_time)
		ELSE NULL::time without time zone
	END) AS thursday_begin_time,
max(
	CASE
		WHEN lch.day_of_week = 4 THEN COALESCE(tgu2.begin_time, tgu.begin_time) + '00:40:00'::interval
		ELSE NULL::time without time zone
	END) AS thursday_end_time,
min(
	CASE
		WHEN lch.day_of_week = 5 THEN COALESCE(tgu2.begin_time, tgu.begin_time)
		ELSE NULL::time without time zone
	END) AS friday_begin_time,
max(
	CASE
		WHEN lch.day_of_week = 5 THEN COALESCE(tgu2.begin_time, tgu.begin_time) + '00:40:00'::interval
		ELSE NULL::time without time zone
	END) AS friday_end_time,
min(
	CASE
		WHEN lch.day_of_week = 6 THEN COALESCE(tgu2.begin_time, tgu.begin_time)
		ELSE NULL::time without time zone
	END) AS saturday_begin_time,
max(
	CASE
		WHEN lch.day_of_week = 6 THEN COALESCE(tgu2.begin_time, tgu.begin_time) + '00:40:00'::interval
		ELSE NULL::time without time zone
	END) AS saturday_end_time,
min(
	CASE
		WHEN lch.day_of_week = 7 THEN COALESCE(tgu2.begin_time, tgu.begin_time)
		ELSE NULL::time without time zone
	END) AS sunday_begin_time,
max(
	CASE
		WHEN lch.day_of_week = 7 THEN COALESCE(tgu2.begin_time, tgu.begin_time) + '00:40:00'::interval
		ELSE NULL::time without time zone
	END) AS sunday_end_time
FROM lesson_course_hours lch
 LEFT JOIN timing_grid_unit tgu ON tgu.timing_grid_id = 0 AND tgu.day_type = 0 AND lch.order_num = tgu.order_num
 LEFT JOIN timing_grid_unit tgu2 ON tgu2.timing_grid_id = 0 AND tgu2.day_type = lch.day_of_week AND lch.order_num = tgu2.order_num
GROUP BY lch.lesson_course_id;

--Представляет расписание как битовую маску
 SELECT lesson_course_hours.lesson_course_id AS id,
    lesson_course_hours.lesson_course_id,
    sum(
        CASE
            WHEN lesson_course_hours.day_of_week = 1 THEN power(2::double precision, lesson_course_hours.order_num::double precision)
            ELSE NULL::double precision
        END) AS monday_bitmask,
    sum(
        CASE
            WHEN lesson_course_hours.day_of_week = 2 THEN power(2::double precision, lesson_course_hours.order_num::double precision)
            ELSE NULL::double precision
        END) AS tuesday_bitmask,
    sum(
        CASE
            WHEN lesson_course_hours.day_of_week = 3 THEN power(2::double precision, lesson_course_hours.order_num::double precision)
            ELSE NULL::double precision
        END) AS wednesday_bitmask,
    sum(
        CASE
            WHEN lesson_course_hours.day_of_week = 4 THEN power(2::double precision, lesson_course_hours.order_num::double precision)
            ELSE NULL::double precision
        END) AS thursday_bitmask,
    sum(
        CASE
            WHEN lesson_course_hours.day_of_week = 5 THEN power(2::double precision, lesson_course_hours.order_num::double precision)
            ELSE NULL::double precision
        END) AS friday_bitmask,
    sum(
        CASE
            WHEN lesson_course_hours.day_of_week = 6 THEN power(2::double precision, lesson_course_hours.order_num::double precision)
            ELSE NULL::double precision
        END) AS saturday_bitmask,
    sum(
        CASE
            WHEN lesson_course_hours.day_of_week = 7 THEN power(2::double precision, lesson_course_hours.order_num::double precision)
            ELSE NULL::double precision
        END) AS sunday_bitmask
   FROM lesson_course_hours
  GROUP BY lesson_course_hours.lesson_course_id;
