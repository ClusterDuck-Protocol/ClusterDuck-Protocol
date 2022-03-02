#ifndef INDEX_H
#define INDEX_H
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <title>ClusterDuck Portal</title>
        <meta name="description" content="ClusterDuck Network Portal by the ClusterDuck Protocol">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
            body {
                font: 14px "Avenir", helvetica, sans-serif;
                -webkit-font-smoothing: antialiased;
            }
            h2 {
                text-align: center;
                color: #111;
                margin: 16px 0 8px;
            }
            h3 {
                font-size: 14px;
                margin: 0 0 24px;
                color: #111;
                font-weight: 400;
            }
            h6 {
                font-size: 12px;
                font-weight: 300;
                line-height: 16px;
                margin: 16px 0 0;
                color: rgba(0,0,0,.5);
            }
            p {
                color: #111;
            }
            .content {
                text-align: center;
                padding: 0 16px;
            }
            .main.on {
               display: block;
            }
            .main.off {
               display: none;
            }
            .main.sent {
            }
            .main.sent .updateBox {
               background: #fff;
               color: #111;
               width: auto;
               max-width: 80%;
               margin: 0 auto;
               padding: 14px;
            }
            .main.sent .updateBox h4 {
               margin: 0 0 1em;
               font-size: 1.5em;
            }
            .updateClass {
               display: block;
               padding: 20px;
               text-align: center;
               cursor: pointer;
            }
            .updateClass:hover {
               opacity: .7;
            }
            .update {
               border: 0;
               background: #fe5454;
            }
            .sendReportBtn {
                box-shadow: 0px 1px 0px 0px #fff6af;
                background:linear-gradient(to bottom, #ffec64 5%, #ffab23 100%);
                background-color:#ffec64;
                border-radius:6px;
                border:1px solid #ffaa22;
                display:block;
                cursor:pointer;
                color:#333333;
                font-family:Arial;
                font-size:15px;
                font-weight:bold;
                padding: 10px 24px;
                text-decoration: none;
                text-shadow: 0px 1px 0px #ffee66;
                text-align: center;
                width: 100%;
                margin-top: 10px;
            }
            .sendReportBtn:hover {
                background:linear-gradient(to bottom, #ffab23 5%, #ffec64 100%);
                background-color:#ffab23;
            }
            .sendReportBtn:active {
                position:relative;
                top:1px;
            }
            #form {
                background-color: #F5F5F5;
                color: #333333;
                width: 100%;
                max-width: 250px;
                margin: auto;
                padding: 18px 24px 14px;
                border-radius: 8px;
                text-align: left;
            }
            .textbox {
                padding: .5em;
                border-radius: 4px;
                border:solid 1px;
                margin: .5em 0;
                display: block;
            }
            .textbox-small {
                width: 20px;
            }
            .textbox-full {
                width: 100%;
                height: 5em;
            }
            label  {
                font-weight: bold;
            }
            .hidden {
                display: none;
            }
        </style>
        </head>
    <body>
        <!-- HTML content of the captive portal below -->
        <h2 class="">You are Connected to a ClusterDuck</h2>
        <div class="content main" id="formContent">
            <h3>Fill out the form below to submit information to the ClusterDuck network.</h3>
            <div id="form">
                <form>
                    <label for="clientId">Your ID:</label>
                    <input type="text" id="clientId" name="clientId" pattern="[A-NPZ1-9]{4}" maxlength=4>
                    <p>ID must be four characters long. It can be any capital letters of the Latin alphabet A-Z except O, or any numbers 1-9.</p>
                    <label for="status">How are you?</label><br />
                    <textarea class="textbox comments textbox-full" name="message" id="commentsInput" cols="30" rows="2"></textarea>
                    <button type="button" id="sendBtn" class="sendReportBtn">Send</button>
                    <p id="makeshiftErrorOutput" class="hidden"></p>
                </form>
                <h6 style="font-size: 10px; text-align: center;margin-top: 24px;">Powered by the ClusterDuck Protocol</h6>
            </div>
        </div>
        <div id="mainSent" class="=main off sent">
            <div class="updateBox">
                <div class="gps"><h4>Message Sent</h4><h5 id="dateNow">March 13, 2019 @ 1:00 PM</h5><p>Your message ID#: XXXXXX</p></div>
                <p class="disclaimer">If your situation changes, please send another update.</p>
                <div id="updateClassupdate" class="updateClass update">Send Update</div>
            </div>
        </div>
        <div id="lastMessageSection" class="hidden">
            <dl>
                <dt>Last message:</dt>
                <dd id="lastMessageField"></dd>
                <dt>Last message ID:</dt>
                <dd id="lastMessageMuid"></dd>
                <dt>Status:</dt>
                <dd id="muidStatus"></dd>
                <dt>Status Message:</dt>
                <dd id="muidStatusMessage"></dd>
            </dl>
        </div>
        <!-- Run javascript actions here -->
        <script type="text/javascript">
            const MUID_URL = '/muidStatus.json';
            const MUID_PARAM_NAME = 'muid';
            const CLIENT_ID_LENGTH = 4;
            const CLIENT_ID_KEY = 'CLIENT_ID';
            const NOT_ACKED_MSG = "The message may have been received, but we're still waiting for confirmation."
            var messageController;
            var muidRequest;
            function CreateMuidRequest(muid) {
                return MuidRequest(
                    muid,
                    document.getElementById('muidStatus'),
                    document.getElementById('muidStatusMessage')
                );
            }
            var MessageController = function() {
                var loadListener = function() {
                    // this.responseText should be something like: {"muid":"ABCD"}
                    var res = JSON.parse(this.responseText);
                    messageController.saveLastMuid(res.muid);
                    var errEl = document.getElementById('makeshiftErrorOutput');
                    if (!errEl.classList.toString().includes("hidden")) {
                        errEl.innerHTML = '';
                        errEl.classList.add("hidden");
                    }
                };
                var errorListener = function() {
                    var errorMessage = 'There was an error sending the message. Please try again.';
                    console.log(errorMessage);
                    var errEl = document.getElementById('makeshiftErrorOutput');
                    errEl.innerHTML = errorMessage;
                    errEl.classList.remove("hidden");
                };
                var showSentMessage = function(commentsInput) {
                    var lastMessageField = document.getElementById('lastMessageField');
                    lastMessageField.innerHTML = commentsInput.value;
                    // TODO: Create a new DOM view so multiple messages can be shown
                    var msgSection = document.getElementById('lastMessageSection');
                    msgSection.classList.remove("hidden");
                };
                return {
                    sendMessage: function() {
                        var clientIdInput = document.getElementById('clientId');
                        var commentsInput = document.getElementById('commentsInput');
                        var params = new URLSearchParams("");
                        params.append(clientIdInput.name, clientIdInput.value);
                        params.append(commentsInput.name, commentsInput.value);
                        var req = new XMLHttpRequest();
                        req.addEventListener("load", loadListener);
                        req.addEventListener("error", errorListener);
                        req.open("POST", "/formSubmit.json?" + params.toString());
                        req.send();
                        showSentMessage(commentsInput);
                    },
                    saveLastMuid: function(muid) {
                        document.getElementById('lastMessageMuid').innerHTML = muid;
                        muidRequest = CreateMuidRequest(muid);
                        muidRequest.requestMuidStatus();
                    },
                };
            };
            var MuidRequest = function(muid, statusEl, messageEl) {
                var showStatus = function(status, message) {
                    statusEl.innerHTML = status;
                    messageEl.innerHTML = message;
                };
                var loadListener = function() {
                    var res = JSON.parse(this.responseText);
                    showStatus(res.status, res.message);
                    if (res.status === 'not_acked') {
                        showStatus('not_acked', NOT_ACKED_MSG)
                        setTimeout(requestMuidStatus, 1000);
                    }
                };
                var errorListener = function() {
                    showStatus('error', 'There was an unknown error');
                    setTimeout(requestMuidStatus, 1000);
                };
                var requestMuidStatus = function() {
                    var req = new XMLHttpRequest();
                    req.addEventListener("load", loadListener);
                    req.addEventListener("error", errorListener);
                    var url = MUID_URL;
                    var params = new URLSearchParams("");
                    params.append(MUID_PARAM_NAME, muid);
                    url += "?" + params.toString();
                    req.open("GET", makeUrlUnique(url));
                    req.send();
                };
                return {
                    requestMuidStatus: requestMuidStatus,
                };
            };
            function makeUrlUnique(url) {
                // Makes the URL bypass the browser's cache.
                // https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest/Using_XMLHttpRequest#bypassing_the_cache
                return url + ((/\?/).test(url) ? "&" : "?") + (new Date()).getTime();
            }
            function storageAvailable(type) {
                var storage;
                try {
                    storage = window[type];
                    var x = '__storage_test__';
                    storage.setItem(x, x);
                    storage.removeItem(x);
                    return true;
                }
                catch(e) {
                    return e instanceof DOMException && (
                        // everything except Firefox
                        e.code === 22 ||
                        // Firefox
                        e.code === 1014 ||
                        // test name field too, because code might not be present
                        // everything except Firefox
                        e.name === 'QuotaExceededError' ||
                        // Firefox
                        e.name === 'NS_ERROR_DOM_QUOTA_REACHED') &&
                        // acknowledge QuotaExceededError only if there's something already stored
                        (storage && storage.length !== 0);
                }
            }
            function generateClientId() {
                var result = '';
                var characters = 'ABCDEFGHIJKLMNPQRSTUVWXYZ123456789';
                for ( var i = 0; i < CLIENT_ID_LENGTH; i++ ) {
                    var randomIndex = Math.floor(Math.random() * characters.length);
                    result += characters.charAt(randomIndex);
               }
               return result;
            }
            function initialize() {
                var clientId;
                if (storageAvailable('localStorage')) {
                    if (window.localStorage.getItem(CLIENT_ID_KEY)) {
                        clientId = window.localStorage.getItem(CLIENT_ID_KEY);
                    } else {
                        clientId = generateClientId();
                        window.localStorage.setItem(CLIENT_ID_KEY, clientId);
                    }
                    document.getElementById('clientId').value = clientId;
                } else {
                    document.getElementById('clientId').value = '';
                }
                messageController = MessageController();
                document.getElementById('sendBtn').addEventListener('click', messageController.sendMessage);
            }
            initialize();
        </script>
    </body>
</html>
)=====";
#endif