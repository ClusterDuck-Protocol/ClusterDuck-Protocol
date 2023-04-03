#ifndef CHAT_PROMPT_H
#define CHAT_PROMPT_H
const char chat_prompt[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
    <head>
        <title>Join Public Chat</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
    </head>

    <body>
        <nav class="title-bar"><span id="title">JOIN PUBLIC CHAT</span></nav>
        <div id="form">
            <form>
                <label for="username">Username: </label><br>
                <textarea pattern="[A-NPZ1-9]{15}" maxlength=15 class="textbox textbox-full" id="username" name="username" placeholder="John" cols="30" rows="2" required></textarea>
                <button type="button" id="sendBtn">Start Chat</button>
                <p id="makeshiftErrorOutput" class="hidden"></p>
            </form>
        </div>

        <script>
            if(sessionStorage.getItem("username")){
                document.getElementById("username").value = sessionStorage.getItem("username");
            }
            function submit(){
                sessionStorage.setItem("username", document.getElementById('username').value);
                window.location.replace("/chat");
            }
            document.getElementById('sendBtn').addEventListener('click', submit);
            document.getElementById("username").focus();
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