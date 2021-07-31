#ifndef HOME_H
#define HOME_H

const char home_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <title>Welcome to DuckLink</title>
    <meta name="viewport" content="width=device-width,initial-scale=1">
  </head>

  <body>
	<h1>DuckLink</h1>
	<p>Welcome to the DuckLink dashboard.  What would you like to do?</p>
	<br>
	<a href="/controlpanel">Control Panel</a>
	<a href="/main">Message Portal</a>
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
	font-size: 20px;
	font-weight: bold;
}

a {
	display: block;
	line-height: 24px;
	font-size: 18px;
	background: #eee;
	padding: 7px 15px;
	margin-bottom: 12px;
	border: 2px solid #ccc;
	border-radius: 5px;
	font-family: 'Arial', sans-serif;
	color: #333;
	font-weight: 600;
	text-decoration: none;
}

</style>
  
)=====";

#endif