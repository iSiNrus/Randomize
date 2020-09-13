import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3


Item {
	function formObj() {
		var obj = {};
		obj.sysuser_type_id = userTypeModel.get(sysuserTypeCombobox.currentIndex).sysuser_type_id;
		obj.sysuser_type = userTypeModel.get(sysuserTypeCombobox.currentIndex).name;
		return obj;
	}

	Rectangle {
		id: rect
		anchors.fill: parent;
		Text {
		  text: "ЗДЕСЬ БУДЕТ ФОРМА ОТЧЕТА"
		  anchors.centerIn: parent
		}
	}
	Component.onCompleted: {
	  reportWindow.width = 300;
	  reportWindow.height = 150;
	}	
}