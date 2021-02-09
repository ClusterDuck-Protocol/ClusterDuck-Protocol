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