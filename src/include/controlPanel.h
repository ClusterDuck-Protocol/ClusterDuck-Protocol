#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

const char controlPanel[] PROGMEM = R"=====(

<!DOCTYPE html><html><head><title>Control Panel</title>
              <style type="text/css">
      .main-box{
        padding:3em;
        font: 26px "Avenir", helvetica, sans-serif;
        border-radius: 8px;
        background-color:#f0f0f0;
        min-height: 100vh;
      }
      a {
        display: block;
        margin-bottom: 14px;
        border: 3px solid #bbb;
        font-weight: 700;
        text-decoration: none;
        background: #f7cf02;
        border-radius: 12px;
        text-align: center;
        padding: .4em;
        color: #000;
        text-transform: uppercase;
      }
      .control-box{
        background-color: white;
        padding: 1em;
        border-radius: 14px;
        margin-bottom: 1em;
      }
      .control-box h5, p{
        margin: 0.2em 0em;
      }
      .submit {
        border-style: none;
        padding: 1em;
        border-radius: 8px;
        background: #f7cf02;
        color: #000;
        text-transform: uppercase;
        font-weight: 700;
        width: 8em;
        margin-bottom:16px;
      }
      .fileupload {
        border-style: none;
        padding: 0.8em;
        border-radius: 8px;
        width:30em;
        background: #e0e0e0;
      }
      h1{
        font-weight: 400;
      }
      input{
        width:100%; 
        height: 3em;  
        border: 3px solid #bbb;
        font-size: 0.8em;
        padding: 0.4em;
        border-radius: 12px;
        background: #f7cf02;
        color: #000;
        text-transform: uppercase;
        font-weight: 700;
      }
      select{
        width: 100%;
        font-size: 1em;
        border: 1px solid black;
        margin-bottom: 0.5em;
      }
      label{
        font-size: 0.83em;
        font-weight: bold;
      }
    </style></head><body>
    <div class="main-box">
     <h1>Control <b>Panel</b></h1>
     <div class="control-box">
       <h5>Channel Select</h5>
       <form action='/setChannel' method='post'>
        <p>Channel:</p>
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
     <a href="/flipDetector">Flip Detector</a>
     <a href="/flipDecrypt">Flip Decrypt</a>
    </div>
  </body></html>
  
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