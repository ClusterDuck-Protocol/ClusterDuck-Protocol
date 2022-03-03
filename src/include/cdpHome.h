#ifndef HOME_H
#define HOME_H

const char home_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <title>ClusterDuck Network</title>
  </head>
  <body>
	<div class="main-box">
		<h1>Welcome to the ClusterDuck Network</h1>
		<p>What would you like to do?</p>
		<br>
		
		<a href="/controlpanel">Control Panel</a>
		<a href="/main">Send Message</a>
	</div>
  </body>
</html>
<style>
body {
	max-width: 85%;
	margin: auto;
	padding-top: 14px;
	padding-bottom: 14px;
}
h1 {
	font-weight: bold;
}
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
</style>
  
)=====";


#endif

// <a href="/wifi">Set WiFi Credentials</a>
// <a href="/main">Message Portal</a>