#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

const char controlPanel[] PROGMEM = R"=====(

<!DOCTYPE html>
<html>
<head>
    <title>Control Panel</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
</head>

<body>
<h2 class="">Control Panel</h2>

<div class="control-box">
    <form action='/setChannel' method='post'>
        <p>Set Channel:</p>
        <select name="channels">
            <option value="1">1</option>
            <option value="2">2</option>
            <option value="3">3</option>
            <option value="4">4</option>
            <option value="5">5</option>
            <option value="6">6</option>
        </select>
        <input type="submit" name="submit"/>
    </form>
</div>
<a href="/flipDetector"><button id="sendBtn">Flip Detector</button></a>
<!-- <a href="/flipDecrypt"><button id="sendBtn">Flip Decryption</button></a> -->
<a href="/"> <button type="button" id="backBtn">Go Back Home</button> </a>




<script>

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

    form{
        padding: 0.5em;
        border-radius: 12px;
        border: 2px solid #2A2C49;
    }
    input{
        font-weight: 700;
        font-size: 14px;
        text-decoration: none;
        border-radius: 12px;
        text-align: center;
        padding: 0.5em;
        color: white;
        background: #2A2C49;
        width: 45%;

    }
    select{
        width: 50%;
        border-radius: 12px;
        padding: 0.5em;
    }

    body {
        padding: 1em;
        font: 26px "Avenir", helvetica, sans-serif;
    }


    </style>
  
)=====";

#endif

// <div class="control-box">
//       <h5>DetectorDuck Settings</h5>
//       <p>Switch to/from DetectorDuck mode.</p>
//       <form action='/flipDetector' method='post'>
//         <input type='submit' value='Detector On/Off' />
//       </form>
//      </div>
// <div class="control-box">
//        <h5>Decryption Settings</h5>
//        <p>Activate decryption on this DuckLink.</p>
//        <form action='/flipDecrypt' method='post'>
//         <input type='submit' value='Turn Decrypt On/Off' />
//        </form>
//      </div>