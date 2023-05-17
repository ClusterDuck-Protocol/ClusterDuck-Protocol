#ifndef HOME_H
#define HOME_H

const char home_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>ClusterDuck Network</title>
</head>
<body>
<div class="main-box">
    <h1>CLUSTERDUCK PROTOCOL</h1>
    <p>Open one of the menus to send a message or read from the Public or a Private chat.</p>
    <br>

    <a id="message-button" href="/main">Send A Message</a>
    <a href="/controlpanel">Control Panel</a>
    <a href="/message-board">View The Message Board</a>
    <a href="/join-chat">Open Global Chat</a>
    <a href="/new-private-chat">Start a Private Chat</a>
</div>
</body>
</html>
<style>

    h1 {
        font-weight: 900;
        color: #2A2C49;
    }

    .main-box {
        padding: 3em;
        font: 26px "Avenir", helvetica, sans-serif;
        border-radius: 8px;
        min-height: 100vh;
    }

    p {
        font-size: 18px;
    }

    a {
        display: block;
        margin-bottom: 32px;
        font-weight: 700;
        font-size: 18px;
        text-decoration: none;
        border-radius: 16px;
        text-align: center;
        padding: 1.5em;
        background: #2A2C49;
        color: #fff;
        text-transform: uppercase;
    }

    #message-button {
        background: #DF3600;
    }
</style>
  
)=====";


#endif

// <a href="/wifi">Set WiFi Credentials</a>
// <a href="/main">Message Portal</a>