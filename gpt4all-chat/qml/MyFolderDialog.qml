import QtCore
import QtQuick
import QtQuick.Dialogs

FileDialog {
    id: folderDialog
    title: qsTr("Please choose a directory")
    fileMode: FileDialog.OpenFile
    options: FileDialog.ShowDirsOnly | FileDialog.ReadOnly

    function openFolderDialog(currentFolder, onAccepted) {
        folderDialog.currentFolder = currentFolder;
        folderDialog.accepted.connect(function() { onAccepted(folderDialog.currentFolder); });
        folderDialog.open();
    }
}
