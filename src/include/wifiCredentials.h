#ifndef WIFIPAGE_H
#define WIFIPAGE_H

const char wifi_page[] PROGMEM = R"=====(


    <!DOCTYPE html><html><head><title>Update WiFi Credentials</title>
              <style type="text/css">

      body{
        padding:4em;
      }        
     
      .main-box{
        padding:2em;
        font: 26px "Avenir", helvetica, sans-serif;
        border-radius: 8px;

      }
      .submit {
            width: 100%;
            margin-top: 10px;
            margin-bottom: 16px;
            font-weight: 700;
            font-size: 14px;
            text-decoration: none;
            border-radius: 12px;
            text-align: center;
            padding: 0.5em;
            background: #2A2C49;
            color: #fff;
            text-transform: uppercase;
            padding: 1em;

      }
      #backBtn {
            width: 100%;
            margin-top: 10px;
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


      input{
        width:98%; 
        height: 3em;  
        font-size: 1.4em;
        border-radius: 6px;
        padding: .35em 0 .35em .3em;
        color: #000;
      }
   
      
    </style></head><body>

    
     <div class="main-box">
     <h1>Update your WiFi credentials</h1>
    <p>Fill in your SSID and password for your local WiFi.</p>
 <form action='/changeSSID' method='post'>
    <label for='ssid'>SSID:</label><br> 
    <input name='ssid' type='text' placeholder='SSID' /><br><br>
    <label for='pass'>Password:</label><br>
    <input name='pass' type='text' placeholder='Password' /><br><br>
    <input class="submit" type='submit' value='Submit' />
 </form>
  <a href="/">
        <button type="button" id="backBtn">Go Back Home</button>
    </a>
   
      </div>
      

  </body></html>
  
)=====";

#endif