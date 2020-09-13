import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3


Item {	

	ListModel {
		id: userTypeModel
		ListElement {
		  sysuser_type_id: 0
		  name: "Суперпользователь"
		}
		ListElement {
		  sysuser_type_id: 1
		  name: "Администратор"
		}
		ListElement {
		  sysuser_type_id: 2
		  name: "Директор"
		}
		ListElement {
		  sysuser_type_id: 3
		  name: "Преподаватель"
		}
		ListElement {
		  sysuser_type_id: 4
		  name: "Ученик"
		}
	}
	
	function formObj() {
		var obj = {};
		obj.sysuser_type_id = userTypeModel.get(sysuserTypeCombobox.currentIndex).sysuser_type_id;
		obj.sysuser_type = userTypeModel.get(sysuserTypeCombobox.currentIndex).name;
		return obj;
	}

	ColumnLayout {
	    anchors.fill: parent
		
		Label {
		  text: "Тип пользователя"
		}
		ComboBox {
		  id: sysuserTypeCombobox
		  Layout.fillWidth: true
		  model: userTypeModel
		  textRole: "name"
		}
		Item {
		  Layout.fillHeight: true
		}
	}		
	Component.onCompleted: {
	  reportWindow.width = 300;
	  reportWindow.height = 150;
	}
}