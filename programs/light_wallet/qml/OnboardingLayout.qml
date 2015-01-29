import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1

import Material 0.1

import "utils.js" as Utils

MainView {
   id: onboarder

   property real minimumWidth: layout.Layout.minimumWidth + visuals.margins * 2
   property real minimumHeight: layout.Layout.minimumHeight + visuals.margins * 2
   property alias username: nameField.text
   property string errorMessage

   signal finished

   function registerAccount() {
      onboarder.state = "REGISTERING"
      Utils.connectOnce(wallet.accounts[username].isRegisteredChanged, finished,
                        function() { return wallet.accounts[username].isRegistered })
      Utils.connectOnce(wallet.onErrorRegistering, function(reason) {
         //FIXME: Do something much, much better here...
         console.log("Can't register: " + reason)
      })

      if( wallet.connected ) {
         wallet.registerAccount(username)
      } else {
         // Not connected. Schedule for when we reconnect.
         wallet.runWhenConnected(function() {
            wallet.registerAccount(username)
         })
      }
   }
   function passwordEntered(password) {
      wallet.createWallet(username, password)
      registerAccount()
   }
   function clearPassword() {
      passwordField.password = ""
   }

   Component.onCompleted: nameField.forceActiveFocus()

   Rectangle {
      anchors.fill: parent
      color: Theme.backgroundColor
   }
   Column {
      id: layout
      anchors.centerIn: parent
      width: parent.width - visuals.margins * 2
      spacing: visuals.margins

      Label {
         anchors.horizontalCenter: parent.horizontalCenter
         horizontalAlignment: Text.AlignHCenter
         text: qsTr("Welcome to BitShares")
         style: "headline"
         wrapMode: Text.WrapAtWordBoundaryOrAnywhere
      }
      Label {
         id: statusText
         text: qsTr("To get started, create a password below.\n" +
                    "This password can be short and easy to remember — we'll make a better one later.")
         anchors.horizontalCenter: parent.horizontalCenter
         width: parent.width
         style: "subheading"
         wrapMode: Text.WrapAtWordBoundaryOrAnywhere
      }
      ColumnLayout {
         width: parent.width

         TextField {
            id: nameField
            input.inputMethodHints: Qt.ImhLowercaseOnly | Qt.ImhLatinOnly
            input.font.pixelSize: units.dp(20)
            placeholderText: qsTr("Pick a Username")
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            text: wallet.accountNames.length? wallet.accountNames[0] : ""
            helperText: defaultHelperText
            characterLimit: 64
            onEditingFinished: if( wallet.connected ) nameAvailable()
            transform: ShakeAnimation { id: nameShaker }

            property string defaultHelperText: qsTr("May contain letters, numbers and hyphens.")

            function nameAvailable() {
               if( text.length === 0 ) {
                  helperText = defaultHelperText
                  displayError = false
               } else if( !wallet.isValidAccountName(text) || text.indexOf('.') >= 0 || wallet.accountExists(text) ) {
                  helperText = qsTr("That username is already taken")
                  displayError = true
               } else if( characterCount > characterLimit ) {
                  helperText = qsTr("Name is too long")
                  displayError = true
               } else {
                  helperText = defaultHelperText
                  displayError = false
               }

               return !displayError
            }
         }
         PasswordField {
            id: passwordField
            Layout.fillWidth: true
            placeholderText: qsTr("Create a Password")
            fontPixelSize: units.dp(20)
            onAccepted: openButton.clicked()

            Connections {
               target: wallet
               onErrorUnlocking: passwordField.shake()
            }
         }
         Button {
            id: openButton
            text: qsTr("Begin")
            Layout.fillWidth: true
            Layout.preferredHeight: passwordField.height

            onClicked: {
               if( wallet.accountNames.length )
                  wallet.accounts[wallet.accountNames[0]].name = nameField.text

               if( !wallet.connected ) {
                  showError("Unable to connect to server.", "Try Again", connectToServer)
                  return
               }

               if( !nameField.nameAvailable() )
                  return nameShaker.shake()

               if( passwordField.password.length < 1 ) {
                  passwordField.shake()
               } else {
                  passwordEntered(passwordField.password)
               }
            }
         }
      }
   }

   states: [
      State {
         name: "REGISTERING"
         PropertyChanges {
            target: openButton
            enabled: false
         }
         PropertyChanges {
            target: statusText
            text: qsTr("OK! Now registering your BitShares Account. Just a moment...")
         }
         PropertyChanges {
            target: wallet
            onErrorRegistering: {
               errorMessage = error
               state = "ERROR"
            }
         }
      },
      State {
         name: "ERROR"
         PropertyChanges {
            target: statusText
            text: errorMessage
            color: "red"
         }
      }
   ]
}
