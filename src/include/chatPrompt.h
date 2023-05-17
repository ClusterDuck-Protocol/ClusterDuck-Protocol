#ifndef CHAT_PROMPT_H
#define CHAT_PROMPT_H
const char chat_prompt[] PROGMEM = R"=====(

<!DOCTYPE html>
<html>
<head>
    <title>Join Global Chat</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
</head>

<body>
<h2 class="">Join private chat</h2>
<p>Enter a username and send a message to other devices connected to the ClusterDuck Protocol network.</p>
<div id="form">
    <form>
        <p>username:</p>
        <textarea pattern="[A-NPZ1-9]{15}" maxlength=15 class="textbox textbox-full" id="username" name="username"
                  placeholder="John" cols="30" rows="2" required></textarea>
        <button type="button" id="sendBtn">Start Chat</button>
        <a href="/">
            <button type="button" id="backBtn">Go Back Home</button>
        </a>

        <p id="makeshiftErrorOutput" class="hidden"></p>
    </form>
</div>

<script>
    if (sessionStorage.getItem("username")) {
        document.getElementById("username").value = sessionStorage.getItem("username");
    }

    function submit() {
        sessionStorage.setItem("username", document.getElementById('username').value);
        window.location.replace("/chat");
    }

    document.getElementById('sendBtn').addEventListener('click', submit);
    document.getElementById("username").focus();
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

    #title {
        font-size: 1.2em;
        top: 20%;
        position: relative;
        font-weight: bold;
    }


    body {
        padding: 1em;
        font: 26px "Avenir", helvetica, sans-serif;
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
</style>

)=====";
#endif