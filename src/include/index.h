#ifndef INDEX_H
#define INDEX_H

const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <title>ClusterDuck Emergency Network</title>
        <meta name="description" content="ClusterDuck Emergency Network Powered by the ClusterDuck Protocol">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
            body {
                background-color: red;
                font: 14px "Avenir", helvetica, sans-serif;
                color: white;
                -webkit-font-smoothing: antialiased;
            }
            .logo {
                width: 100px;
                padding-top: 16px;
            }
            .logo-container {
                text-align: center;
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
               padding: 1em;
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
            .lang, .details, .update, .home {
               display: block;
               color: #fff;
               border: 1px solid #fff;
               font-size: 1.25em;
               font-weight: 600;
               line-height: 1.5em;
               margin-bottom: 1em;
            }
            .details {
               background: #000000;
               border: 0;
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
                display:inline-block;
                cursor:pointer;
                color:#333333;
                font-family:Arial;
                font-size:15px;
                font-weight:bold;
                padding:6px 24px;
                text-decoration:none;
                text-shadow:0px 1px 0px #ffee66;
                text-align: center;
                width: 210px;
                margin-top: 24px;
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
                padding: 32px;
                border-radius: 8px;
                text-align: left;
            }
            .textbox {
                padding: 4px;
                border-radius: 4px;
                border:solid 1px;
                /* color: rgb(184, 184, 184); */
            }
            .textbox-small {
                width: 20px;
            }
            .textbox-full {
                width: 100%;
            }
            .comments {
                width: 100%;
                height: 100px;
            }
            label  {
                font-weight: bold;
            }

        </style>
        </head>
    <body>
        <div class="logo-container">
            <img class="logo" src='data:image/png;base64,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' alt=''>
            <h2 class="">ClusterDuck Emergency Network</h2>
        </div>
        <div class="content body" id="formContent">
            <p>You are connected to a ClusterDuck Emergency Network. Please fill out the form to share your status with first responders.</p>
            <div id="form">
                <form action="/formSubmit" method="post">
                    <label for="firstName">First Name</label><br>
                    <input class="textbox textbox-full" name="firstName" id="firstName" type="text" placeholder="Tarzán" /><br><br>
                    <label for="lastName">Last Name(s)</label><br>
                    <input class="textbox textbox-full" name="lastName" id="lastName" type="text" placeholder="Bulldog" /> <br /><br>
                    <label for="streetAddress">Street Address</label>
                    <input class="textbox textbox-full" name="streetAddress" id="streetAddress" type="text" placeholder="PR-108" /> <br /> <br />
                    <label for="city">City</label><br>
                    <input class="textbox textbox-full" name="city" type="text" placeholder="Mayagüez" /><br><br>
                    <label for="zipcode">Zipcode</label><br>
                    <input class="textbox textbox-full" name="zipcode" type="number" placeholder="00682" /> <br /> <br />
                    <label for="phone">Phone</label><br />
                    <input class="textbox textbox-full" name="phone" id="phoneNumber" type="tel" placeholder="787-123-1234" />
                    <br /> <br />
                    <label for="status">Current Status</label><br />
                    <input name="status" id="currentStatusSos" type="radio" value="sos" /> I'm in immediate danger (SOS)
                    <br><input name="status" id="currentStatusOk" type="radio" value="ok" /> OK <br />
                    <br />
                    <label for="status">Needs</label><br />
                    <input type="checkbox" name="water" id="waterNeed" type="radio" value="water" /> Water
                    <input name="transportation" id="transportationNeed" type="checkbox" value="transportation" /> Transportation <br />
                    <input type="checkbox" name="food" id="foodNeed" type="radio" value="food" /> Food
                    <input name="inspection" id="inspectionNeed" type="checkbox" value="inspection" /> Structural Inspection <br />
                    <input type="checkbox" name="firstaid" id="firstAidNeed" type="radio" value="firstaid" /> First Aid
                    <input name="shelter" type="checkbox" id="shelterNeed" value="shelter" /> Shelter <br />
                    <br />
                    <label for="status">How many people are with you?</label><br />
                    <span>Adults </span><input class="textbox textbox-small" name="adults" id="adultsInput" type="number" placeholder="2" />
                    <span>Children </span><input class="textbox textbox-small" name="children" id="childrenInput" type="number" placeholder="2" />
                    <span>Elderly </span><input class="textbox textbox-small" name="elderly" id="elderlyInput" type="number" placeholder="2" /><br><br>
                    <label for="status">Do you have any pets?</label><br />
                    <input class="textbox textbox-full" name="pets" id="petsInput" type="text" placeholder="1 gato y 1 perro" />
                    <br /> <br />
                    <label for="status">Additional Comments</label><br />
                    <textarea class="textbox comments" name="message" id="commentsInput" cols="30" rows="2"></textarea><br />
                    <input type="submit" class="sendReportBtn" value="SEND REPORT" />
                </form>
                <p style="font-size: 10px; text-align: center;margin-top: 24px;">Powered by the ClusterDuck Protocol</p>
            </div>
        </div>
      <div id="bodySent" class="body off sent">
         <div class="c">
            <div class="gps"><h4>Message Sent</h4><h5 id="dateNow">March 13, 2019 @ 1:02 PM</h5><p>Your message ID#: 9XP002</p></div>
            <p class="disclaimer">If your situation changes, please send another update.
            <div id="bupdate" class="b update">Send Update</div>
         </div>
      </div>
      <script type="text/javascript">
      </script>
    </body>
</html>
)=====";

#endif