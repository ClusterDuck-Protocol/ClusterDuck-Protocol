#ifndef MESSAGE_BOARD_H
#define MESSAGE_BOARD_H
const char message_board[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <title>Message Board</title>
        <meta name="description" content="ClusterDuck Network Portal by the ClusterDuck Protocol">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
            <div id="messages-container"></div>
        </style>
    </head>
    <body onload="displayNewMessage("first")">
        <script>
            function displayNewMessage(newMessage){
                var span = document.createElement("span");
                span.append(newMessage);
                document.getElementById('messages-container').appendChild(span);
            }

            if (!!window.EventSource) {
                var source = new EventSource('/events');

                source.addEventListener('open', function(e) {
                    console.log("Events Connected");
                }, false);

                source.addEventListener('error', function(e) {
                    if (e.target.readyState != EventSource.OPEN) {
                    console.log("Events Disconnected");
                    }
                }, false);

                source.addEventListener('newMessage', function(newMessage) {
                    console.log(newMessage);
                    displayNewMessage(newMessage.data);
                }, false);
            }
        </script>
    </body>
</html>
)=====";
#endif