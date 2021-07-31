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
        height: 100vh;
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
        background: #05172d;
        color: white;
        width: 8em;
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
        border: 0.3px solid lightgray;
        font-size: 0.8em;
        padding: 0.4em;
        border-radius: 12px;

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
      <h5>DetectorDuck </h5>
      <p>Switch between DetectorDuck and normal duck</p>
        <form action='/flipDetector' method='post'>
    <input type='submit' value='Detector On' />
    </form>
    <form action='/flipDecrypt' method='post'>
    <input type='submit' value='Decrypt On' />
    </form>
     </div>
     <div class="control-box">
      <h5>Channel Select</h5>
      <p>Channel Select</p>
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
    </div>
  </body></html>
  
)=====";

#endif