#ifndef WIFIPAGE_H
#define WIFIPAGE_H

const char wifi_page[] PROGMEM = R"=====(


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
     <h2>Use this page to update your Wifi credentials</h1>
    <p>Fill in your SSID and WiFi Password in the form below.</p>
 <form action='/changeSSID' method='post'>
    <label for='ssid'>SSID:</label><br> 
    <input name='ssid' type='text' placeholder='SSID' /><br><br>
    <label for='pass'>Password:</label><br>
    <input name='pass' type='text' placeholder='Password' /><br><br>
    <input type='submit' value='Submit' />
 </form>
 
   
      </div>

  </body></html>
  
)=====";

#endif