#ifndef CHAT_PAGE_H
#define CHAT_PAGE_H

const char chat_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
    <head>
        <title>Global Chat</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
    </head>

    <body>
    <nav class="title-bar"><span id="title">GLOBAL NETWORK CHAT</span></nav>
        <div id="message-container"></div>
        <div class="chat-bar">
            <div id="form">
                <form>
                    <input type="text" name="chatMessage" id="chatMessage" placeholder="Your Message"></input>
                    <button type="button" id="sendBtn" class="sendBtn">Send</button>
                    <p id="makeshiftErrorOutput" class="hidden"></p>
                </form>
            </div>
        </div>

        <script>
            var sduid = "you";
             function displayNewMessage(newMessage, sent){
                newMessage.messageAge = parseInt(newMessage.messageAge / 1000);
                var card = document.createElement("div");
                if(sent){
                    card.classList.add("sent-message-card");
                }else{
                    card.classList.add("received-message-card");
                }
                card.innerHTML = newMessage.body + '</p><span class="duid">FROM DUCKID: '
                 + newMessage.sduid + '</span></p><span class="time">' 
                 + newMessage.messageAge + ' seconds ago</span>';

                document.getElementById('message-container').prepend(card);
            }
            function chatHistoryListener () {
                var data = JSON.parse(this.responseText);
                data.posts.forEach(item => {
                        let sent = sduid == item.sduid ? true : false;
                        displayNewMessage(item, sent);
                    });
                
            }
            function sduidListener () {
                sduid = JSON.parse(this.responseText);
            }

            function requestChatHistory(){
                var req = new XMLHttpRequest();
                req.addEventListener("load", chatHistoryListener);
                req.open("GET", "/chatHistory");
                req.send();
            }

            function requestSduid(){
                var req = new XMLHttpRequest();
                req.addEventListener("load", sduidListener);
                req.open("GET", "/id");
                req.send();
            }



            function loadListener(){
                console.log('request returned');
                var errEl = document.getElementById('makeshiftErrorOutput');
                if (!errEl.classList.toString().includes("hidden")) {
                    errEl.innerHTML = '';
                    errEl.classList.add("hidden");
                }
            };
            function errorListener(){
                var errorMessage = 'There was an error sending the message. Please try again.';
                console.log(errorMessage);
                var errEl = document.getElementById('makeshiftErrorOutput');
                errEl.innerHTML = errorMessage;
                errEl.classList.remove("hidden");
            };
            function sendMessage(){
                let message = document.getElementById('chatMessage');
                let params = new URLSearchParams("");
                params.append(message.name, message.value);
                var req = new XMLHttpRequest();
                req.addEventListener("load", loadListener);
                req.addEventListener("error", errorListener);
                req.open("POST", "/chatSubmit.json?" + params.toString());
                req.send();
                displayNewMessage({body: message.value, messageAge: 0, sduid: sduid}, true);
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
                source.addEventListener('refreshPage', function(data) {
                    location.reload(); 
                }, false);

            }
            document.getElementById('sendBtn').addEventListener('click', sendMessage);
            
            requestSduid();
            requestChatHistory();
        </script>
    </body>
</html>
<style>
    .received-message-card{
        border-radius: 10px;
        background-color: #F0EEF7;
        padding: 0 3vw 1vh;
        margin: 2vw;
        overflow:auto;
        width: 70%;
        padding-top: 5px;
    }
    .sent-message-card{
        border-radius: 10px;
        background-color: #ffff004f;
        padding: 0 3vw 1vh;
        margin: 2vw;
        overflow:auto;
        width: 70%;
        float: right;
        padding-top: 5px;
    }
    #message-container{
        width: 95%;
        margin: 10vh auto;
    }
    .time{
        margin-right: 3vw;
        float: right;
        color: gray;
        font-size: 0.8em;
    }
    .duid{
        margin-left: 3vw;
        float: left;
        color: gray;
        font-size: 0.8em;
    }
    #title{
        font-size: 1.2em;
        top: 20%;
        position: relative;
        font-weight: bold;
    }
    .title-bar{
        background-color: black;
        box-shadow: 0 2px 3px gray;
        height: 35px;
        position: fixed;
        top: 0%;
        width: 100%;
        color: white;
        left: 0%;
        text-align: center;
    }
    .chat-bar{
        background-color: #9b9389;
        height: 35px;
        position: fixed;
        bottom: 0;
        width: 100%;
        padding: 5px;
        left: 0%;
    }
    #chatMessage{
        width: 70%;
    }
    #sendBtn{
        background-color: #ffe700;
        border-radius: 5px;
        border-style: none;
        height: 34px;
        width: 100px;
        float: right;
        margin-right: 15px;
    }
    body{
        font-family: sans-serif;
        font-size: .9em;
    }

</style>
)=====";
#endif