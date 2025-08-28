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
            padding: 2em;
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
            color: rgba(0, 0, 0, .5);
        }

        p {
            color: #111;
        }

        a {
            text-decoration: none;
        }

        .content {
            text-align: center;
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
            background-color: #2A2C49;
            border-radius: 6px;
            display: block;
            cursor: pointer;
            color: white;
            font-family: Arial;
            font-size: 15px;
            font-weight: 700;
            padding: 10px 24px;
            text-decoration: none;
            text-align: center;
            margin-top: 10px;
        }

        .updateBox {
            background-color: #F5F5F5;
            color: #333333;
            padding: 18px 24px 14px;
            border-radius: 8px;
            text-align: left;
        }

        #sendBtn {
            width: 100%;
            margin-top: 10px;
            margin-bottom: 32px;
            font-weight: 700;
            font-size: 14px;
            text-decoration: none;
            border-radius: 12px;
            text-align: center;
            padding: 0.5em;
            background: #2A2C49;
            color: #fff;
            text-transform: uppercase;

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

        .sendReportBtn:hover {
            background: #E1E1E1;
            background-color: #ffab23;
        }

        .sendReportBtn:active {
            position: relative;
            top: 1px;
        }

        #form {
            background-color: #F5F5F5;
            color: #333333;
            padding: 18px 24px 14px;
            border-radius: 8px;
            text-align: left;
            margin-bottom: 2em;
        }

        .textbox {
            padding: .5em;
            border-radius: 4px;
            border: solid 1px;
            margin: .5em 0;
            display: block;
        }

        .textbox-small {
            width: 20px;
        }

        .textbox-full {
            width: 95%;
            height: 5em;
        }

        label {
            font-weight: bold;
        }

        .hidden {
            display: none;
        }

        #lastMessageSection {
            font-weight: 400;
        }
    </style>
</head>
<body>
<!-- HTML content of the captive portal below -->
<h2 class="">Send A Message</h2>
<div class="content main" id="formContent">
    <h3>Fill out the form below to submit information to the ClusterDuck network.</h3>
    <div id="form">
        <form>
            <label for="clientId">Your ID:</label>
            <input type="text" id="clientId" name="clientId" pattern="[A-NPZ1-9]{4}" maxlength=4>
            <p>ID must be four characters long. It can be any capital letters of the Latin alphabet A-Z except O, or any
                numbers 1-9.</p>
            <label for="status">Your message:</label><br/>
            <textarea class="textbox comments textbox-full" maxlength=200 name="message" id="commentsInput" cols="30"
                      rows="2" onkeyup="updateCounter(this)"></textarea>

                    Remaining characters: <span id="counter">200</span>

                    <script type="text/javascript">
                    function updateCounter(textarea) {
                        var currentLength = textarea.value.length;
                        var maxCharacters = 200;
                        var remainingCharacters = maxCharacters - currentLength;
                        document.getElementById("counter").innerHTML = remainingCharacters;  
                    }
                    </script>
            </textarea>
                      
            <button type="button" id="sendBtn" class="sendReportBtn">Send</button>
            <p id="makeshiftErrorOutput" class="hidden"></p>
        </form>
    </div>
</div>
<div id="mainSent" class="=main off sent">
    <div class="updateBox">
        <div class="gps"><h4>Message Sent</h4>
            <h5 id="dateNow">
                <p class="disclaimer">If your situation changes, please send another update.</p>
                <div id="lastMessageSection" class="hidden">
                    <dl>
                        <dt>Last message:</dt>
                        <dd id="lastMessageField"></dd>
                    </dl>
                </div>

                <div id="messages-container"></div>
        </div>
    </div>
    <a href="/">
        <button type="button" id="backBtn">Go Back Home</button>
    </a>


    <!-- Run javascript actions here -->
    <script type="text/javascript">
        const MUID_PARAM_NAME = 'muid';
        const CLIENT_ID_LENGTH = 4;
        const CLIENT_ID_KEY = 'CLIENT_ID';
        const NOT_ACKED_MSG = "The message may have been received, but we're still waiting for confirmation."
        var messageController;
        var muidRequest;


        var MessageController = function () {
            var loadListener = function (req) {
                var errEl = document.getElementById('makeshiftErrorOutput');
                if (!errEl.classList.toString().includes("hidden")) {
                    errEl.innerHTML = '';
                    errEl.classList.add("hidden");
                }
                var res = JSON.parse(req.responseText);
            };
            var errorListener = function () {
                var errorMessage = 'There was an error sending the message. Please try again.';
                console.log(errorMessage);
                var errEl = document.getElementById('makeshiftErrorOutput');
                errEl.innerHTML = errorMessage;
                errEl.classList.remove("hidden");
            };
            var showSentMessage = function (commentsInput) {
                var lastMessageField = document.getElementById('lastMessageField');
                lastMessageField.innerHTML = commentsInput.value;
                // TODO: Create a new DOM view so multiple messages can be shown
                var msgSection = document.getElementById('lastMessageSection');
                msgSection.classList.remove("hidden");
            };
            return {
                sendMessage: function () {
                    var clientIdInput = document.getElementById('clientId');
                    var commentsInput = document.getElementById('commentsInput');
                    var params = new URLSearchParams("");
                    params.append(clientIdInput.name, clientIdInput.value);
                    params.append(commentsInput.name, commentsInput.value);
                    var req = new XMLHttpRequest();
                    req.addEventListener("load", function() { loadListener(req); });
                    req.addEventListener("error", errorListener);
                    req.open("POST", "/formSubmit.json?" + params.toString());
                    req.send();
                    showSentMessage(commentsInput);
                }
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
            } catch (e) {
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
            for (var i = 0; i < CLIENT_ID_LENGTH; i++) {
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