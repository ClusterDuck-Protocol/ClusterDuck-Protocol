#ifndef EMERGENCYPORTAL_H
#define EMERGENCYPORTAL_H

const char emergencyPortal[] PROGMEM = R"=====(
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
                background-color: black;
                font: 14px "Avenir", helvetica, sans-serif;
                color: white;
                -webkit-font-smoothing: antialiased;
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
            #sendReportBtn {
                box-shadow: 0px 1px 0px 0px #fff6af;
                background:linear-gradient(to bottom, #ffec64 5%, #ffab23 100%);
                background-color:#ffec64;
                border-radius:6px;
                border:1px solid #ffaa22;
                cursor:pointer;
                color:#333333;
                font-family:Arial;
                font-size:15px;
                font-weight:bold;
                padding:6px 24px;
                text-decoration:none;
                text-shadow:0px 1px 0px #ffee66;
                text-align: center;
                width: 100%;
                margin-top: 24px;
                display: none;

            }
            #sendReportBtn:hover {
                background:linear-gradient(to bottom, #ffab23 5%, #ffec64 100%);
                background-color:#ffab23;
            }
            #sendReportBtn:active {
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
            .alert{
                color:red;
                font-weight: 800;
            }
            label  {
                font-weight: bold;
            }
            #sendReportBtn.button-on{
                display: block;

            }
           

        </style>
        </head>
    <body>
   
        <div class="content body" id="formContent">
            <h2 class="">ClusterDuck Emergency Network</h2>
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
                    <input class="textbox textbox-full" id="city" name="city" type="text" placeholder="Mayagüez" /><br><br>
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
                    <div id="count">
                    
                    </div>
                    <input type="submit"  class="button-on" id="sendReportBtn" value="SEND REPORT" />
                </form>
                <p style="font-size: 10px; text-align: center;margin-top: 24px;">Powered by the ClusterDuck Protocol</p>
            </div>
        </div>
      <script type="text/javascript">


        document.onkeydown = function () {
            var AllLength = document.getElementById("firstName").value + document.getElementById("lastName").value + document.getElementById("streetAddress").value + document.getElementById("city").value + document.getElementById("commentsInput").value + document.getElementById("petsInput").value;
                if (130 > AllLength.length) {
                    document.getElementById("count").innerHTML='Length ' + AllLength.length + "/130";
                    document.getElementById("sendReportBtn").classList.add("button-on");
                     document.getElementById("count").classList.remove("alert");      
                }   else{
                     document.getElementById("count").innerHTML='Length ' + AllLength.length + "/130";
                      document.getElementById("count").classList.add("alert");
                     document.getElementById("sendReportBtn").classList.remove("button-on");
                }
            };
      </script>
    </body>
</html>
)=====";

#endif