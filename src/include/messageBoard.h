#ifndef MESSAGE_BOARD_H
#define MESSAGE_BOARD_H

const char message_board[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
    <head>
        <title>Message Board</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
    </head>

    <body>
        <div id="message-container">
            <h5>MOST RECENT</h5>
        </div>

        <script>
             function displayNewMessage(newMessage){
                newMessage.messageAge = parseInt(newMessage.messageAge / 1000);
                var card = document.createElement("div");
                card.classList.add("message-card"); 
                card.innerHTML = '<h4>' + newMessage.title + '</h4><p>' 
                + newMessage.body + '</p><span class="time">' + newMessage.messageAge + 'seconds ago</span>';

                document.getElementById('message-container').prepend(card);
            }
            function messageHistoryListener () {
                var data = JSON.parse(this.responseText);
                data.posts.forEach(item => displayNewMessage(item));
                
            }

            function requestMessageHistory(){
                    var req = new XMLHttpRequest();
                    req.addEventListener("load", messageHistoryListener);
                    req.open("GET", "/messageHistory");
                    req.send();
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
                    displayNewMessage(newMessage.data);
                }, false);
            }
            
            requestMessageHistory();
        </script>
    </body>
</html>
<style>
    .message-card{
        border-radius: 10px;
        border-style: solid;
        border-color: lightgrey;
        border-width: 2px;
        padding: 0 3vw 1vh;
        margin: 2vw;
        overflow:auto;
    }
    #message-container{
        width: 95%;
        margin: 10vh auto;
    }
    .time{
        margin-right: 3vw;
        float: right;
        color: gray;
    }
    body{
        font-family: sans-serif;
        font-size: .9em;
    }
</style>
)=====";
#endif