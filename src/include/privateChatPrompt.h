#ifndef PRIVATE_CHAT_PROMPT_H
#define PRIVATE_CHAT_PROMPT_H
const char private_chat_prompt[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
    <head>
        <title>New Private Chat</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
    </head>

    <body>
        <nav class="title-bar"><span id="title">NEW PRIVATE CHAT</span></nav>
        <div id="form">
            <form>
                <label for="dduid">Which device do you want to start a chat with?</label><br>
                <textarea pattern="[A-NPZ1-9]{8}" maxlength=8 class="textbox textbox-full" id="dduid" name="dduid" placeholder="MAMA0001" cols="30" rows="2"></textarea>
                <button type="button" id="sendBtn">Start Chat</button>
                <p id="makeshiftErrorOutput" class="hidden"></p>
            </form>
        </div>
        <h3>Chat Histories</h3>
        <div id="histories">
        </div>

        <script>
            function submit(){
                let dduidInput = document.getElementById('dduid');
                let params = new URLSearchParams("");
                params.append(dduidInput.name, dduidInput.value);
                let req = new XMLHttpRequest();
                req.addEventListener("load", openChatListener);
                req.addEventListener("error", errorListener);
                req.open("POST", "/openPrivateChat.json?" + params.toString());
                req.send();
            }
            function openChatListener(){
                window.location.replace("/private-chat");
            };
            function errorListener(){
                let errorMessage = 'There was an error starting the chat. Please try again.';
                console.log(errorMessage);
                let errEl = document.getElementById('makeshiftErrorOutput');
                errEl.innerHTML = errorMessage;
                errEl.classList.remove("hidden");
            };
            function openChatHistory(dduid){
                let params = new URLSearchParams("");
                params.append("dduid", dduid);
                let req = new XMLHttpRequest();
                req.addEventListener("load", openChatListener);
                req.addEventListener("error", errorListener);
                req.open("POST", "/openPrivateChat.json?" + params.toString());
                req.send();
            };
            function createHistoryButton(btnName){
                var button = document.createElement("button");
                button.classList.add("historyBtn");
                button.innerHTML = btnName;
                document.getElementById('histories').prepend(button);
                button.onclick = ()=>openChatHistory(btnName);
            }
            function historiesListener () {
                console.log(this.responseText);
                var data = JSON.parse(this.responseText);
                data.histories.forEach(item => {
                        createHistoryButton(item);
                    });
            }
            function retrieveHistories(){
                var req = new XMLHttpRequest();
                req.addEventListener("load", historiesListener);
                req.open("GET", "/privateChatHistories");
                req.send();
            }
            retrieveHistories();
            document.getElementById('sendBtn').addEventListener('click', submit);
            document.getElementById("dduid").focus();
        </script>
    </body>
</html>


<style>
    #form{
        background-color: #ffe421;
        max-width: 250px;
        margin: auto;
        padding: 18px 24px 14px;
        border-radius: 3px;
        text-align: left;
        margin-top: 60px;
    }
    .textbox {
        border-radius: 4px;
        border: none;
        margin: .5em 0;
    }
    .textbox-full {
        width: 100%;
        height: 5em;
    }
    #sendBtn {
        background-color: #f4f4ed;
        border-radius: 5px;
        border: none;
        cursor: pointer;
        color: #333333;
        font-size: 15px;
        font-weight: bold;
        padding: 8px;
        text-align: center;
        width: 100%;
        margin-top: 10px;
    }
    #sendBtn:hover {
        background-color:#ccc;
    }
    #sendBtn:active {
        position:relative;
        top:1px;
    }
    .historyBtn {
        background-color: #ffe421;
        max-width: 250px;
        margin: auto;
        padding: 18px 24px 14px;
        border-radius: 3px;
    }
    #histories{

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
    body{
        font-family: sans-serif;
        font-size: .9em;
    }
</style>
)=====";
#endif