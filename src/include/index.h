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
                color: white;
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
            .body.on {
               display: block;
            }
            .body.off {
               display: none;
            }
            .body.sent {
            }
            .body.sent .c {
               background: #fff;
               color: #111;
               width: auto;
               max-width: 80%;
               margin: 0 auto;
               padding: 14px;
            }
            .body.sent .c h4 {
               margin: 0 0 1em;
               font-size: 1.5em;
            }
            .b {
               display: block;
               padding: 20px;
               text-align: center;
               cursor: pointer;
            }
            .b:hover {
               opacity: .7;
            }
            .logo {
                width: 75%;
                padding-top: 16px;
            }
            .logo-container {
                text-align: center;
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
        </style>
        </head>
    <body>
    <div class="logo-container">
            <img class="logo" src='data:image/png;base64,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'>
    </div>
        <!-- HTML content of the captive portal below -->
        <h2 class="">You are Connected to a ClusterDuck</h2>
        <div class="content body" id="formContent">
            <h3>Fill out the form below to submit information to the ClusterDuck network.</h3>
            <div id="form">
                <form action="/formSubmit" method="post">
                    <label for="status">How are you?</label><br />
                    <textarea class="textbox comments textbox-full" name="message" id="commentsInput" cols="30" rows="2"></textarea>
                    <input type="submit" class="sendReportBtn" value="Submit" />
                </form>
                <h6 style="font-size: 10px; text-align: center;margin-top: 24px;">Powered by the ClusterDuck Protocol</h6>
            </div>
        </div>
        <div id="bodySent" class="body off sent">
            <div class="c">
                <div class="gps"><h4>Message Sent</h4><h5 id="dateNow">March 13, 2019 @ 1:00 PM</h5><p>Your message ID#: XXXXXX</p></div>
                <p class="disclaimer">If your situation changes, please send another update.</p>
                <div id="bupdate" class="b update">Send Update</div>
            </div>
        </div>
      <!-- Run javascript actions here -->
      <script type="text/javascript"></script>
    </body>
</html>
)=====";
#endif