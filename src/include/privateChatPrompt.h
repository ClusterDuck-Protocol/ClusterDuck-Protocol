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
<h2 class="">Join private chat</h2>
<div id="form">
    <form>
        <p>Which device do you want to start a chat with? (Enter DeviceID)</p>
        <textarea pattern="[A-NPZ1-9]{8}" maxlength=8 class="textbox textbox-full" id="dduid" name="dduid"
                  placeholder="MAMA0001" cols="30" rows="2" required></textarea>
        <p>Username:</p>
        <textarea pattern="[A-NPZ1-9]{15}" maxlength=15 class="textbox textbox-full" id="username" name="username"
                  placeholder="John" cols="30" rows="2" required></textarea>
        <button type="button" id="sendBtn">Start Chat</button>
        <a href="/">
            <button type="button" id="backBtn">Go Back Home</button>
        </a>
        <p id="makeshiftErrorOutput" class="hidden"></p>
    </form>
</div>
<p>Chat Histories</p>
<div id="histories">
</div>

<script>
    if (sessionStorage.getItem("username")) {
        document.getElementById("username").value = sessionStorage.getItem("username");
    }

    //actually save the name in the chatbuffer
    function submit() {
        let dduidInput = document.getElementById('dduid');
        sessionStorage.setItem("username", document.getElementById('username').value);
        let params = new URLSearchParams("");
        params.append(dduidInput.name, dduidInput.value);
        let req = new XMLHttpRequest();
        req.addEventListener("load", openChatListener);
        req.addEventListener("error", errorListener);
        req.open("POST", "/openPrivateChat.json?" + params.toString());
        req.send();
    }

    function openChatListener() {
        window.location.replace("/private-chat");
    };

    function errorListener() {
        let errorMessage = 'There was an error starting the chat. Please try again.';
        console.log(errorMessage);
        let errEl = document.getElementById('makeshiftErrorOutput');
        errEl.innerHTML = errorMessage;
        errEl.classList.remove("hidden");
    };

    function openChatHistory(dduid) {
        let params = new URLSearchParams("");
        params.append("dduid", dduid);
        let req = new XMLHttpRequest();
        req.addEventListener("load", openChatListener);
        req.addEventListener("error", errorListener);
        req.open("POST", "/openPrivateChat.json?" + params.toString());
        req.send();
    };

    function createHistoryButton(btnName) {
        var button = document.createElement("button");
        button.classList.add("historyBtn");
        button.innerHTML = btnName;
        document.getElementById('histories').prepend(button);
        button.onclick = () => openChatHistory(btnName);
    }

    function historiesListener() {
        var data = JSON.parse(this.responseText);
        data.histories.forEach(item => {
            createHistoryButton(item);
        });
    }

    function retrieveHistories() {
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
    h2 {
        text-align: center;
        color: #111;
        margin: 16px 0 8px;
        font-size: 26px;
    }

    p {
        font-size: 16px;
    }

    #form {
        margin-top: 60px;
    }

    .textbox {
        border-radius: 4px;
        border: none;
        margin: .5em 0;
        border-radius: 12px;
        border: 2px solid #2A2C49;
        padding: 0.5em;
    }

    .textbox-full {
        height: 5em;
        width: 95%;
    }

    #sendBtn {
        width: 100%;
        margin-top: 10px;
        font-weight: 700;
        font-size: 14px;
        text-decoration: none;
        border-radius: 12px;
        text-align: center;
        padding: 0.5em;
        color: white;
        background: #2A2C49;
    }

    #sendBtn:hover {
        background-color: #ccc;
    }

    #sendBtn:active {
        position: relative;
        top: 1px;
    }

    .historyBtn {
        background-color: #2A2C49;
        max-width: 250px;
        margin: auto;
        padding: 18px 24px 14px;
        border-radius: 3px;
        color:white;
    }

    #histories {
    color:white;

    }

    #title {
        font-size: 1.2em;
        top: 20%;
        position: relative;
        font-weight: bold;
    }

    .title-bar {
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

    #backBtn {
        width: 100%;
        margin-top: 10px;
        margin-bottom: 32px;
        font-weight: 700;
        font-size: 14px;
        text-decoration: none;
        border-radius: 12px;
        text-align: center;
        padding: 0.5em;
        color: #2A2C49;
        background: white;
        border: 2px solid #2A2C49;

    }

    body {
        padding: 1em;
        font: 26px "Avenir", helvetica, sans-serif;
    }
</style>
)=====";
#endif