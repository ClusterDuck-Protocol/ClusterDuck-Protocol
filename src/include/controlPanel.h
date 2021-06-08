#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

const char controlPanel[] PROGMEM = R"=====(


   <!DOCTYPE html><html><head><title>Update Wifi Credentials</title>
              <style type="text/css">
     
      .main-box{
        padding:3em;
        font: 26px "Avenir", helvetica, sans-serif;
        height: 50vh;
        border-radius: 8px;

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
      img{
        width:100%;
        margin-top: 10  em;  
      }
      input{
        width:100%; 
        height: 5em;  
      }
   
      
    </style></head><body>

    
     <div class="main-box">
     <h1>Control Panel</h1>
    <form action='/flipDetector' method='post'>
    <input type='submit' value='Detector On' />
    </form>
    <p>Channel Select</p>
    <form action='/setChannel' method='post'>

    <input type="radio" id="channel1" name="channel" value=1>
    <label for="channel1">1</label><br>

    <input type="radio" id="channel2" name="channel" value=2>
    <label for="channel2">2</label><br>

    <input type="radio" id="channel3" name="channel" value=3>
    <label for="channel3">3</label><br>

    <input type="radio" id="channel4" name="channel" value=4>
    <label for="channel4">4</label><br>

    <input type="radio" id="channel5" name="channel" value=5>
    <label for="channel5">5</label><br>

    <input type="radio" id="channel6" name="channel" value=6>
    <label for="channel6">6</label><br>
    <input type="submit" value="Submit">
    </form>
    
 
   
      </div>

  </body></html>
  
)=====";

#endif