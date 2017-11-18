const char SETDATE_page[] PROGMEM = R"=====(
<HTML>
<TITLE>Date</TITLE>
<BODY>
<a href="./" style="font-size:16pt">Главная</a>>Смена даты<br><br>
<FORM method="post">
Year:<INPUT TYPE=number value=@@YEAR@@ min=2000 max=2100  name=year>
Month:<INPUT TYPE=number value=@@MONTH@@ min=1  max=12  name=month>
Day:<INPUT TYPE=number value=@@DAY@@ min=1 max=31 name=day>
Hour:<INPUT TYPE=number value=@@HOUR@@ min=0 max=23 name=hour>
Minute:<INPUT TYPE=number value=@@MINUTE@@ min=0 max=59  name=minute>
<INPUT TYPE=SUBMIT name=submit>
</FORM>
<h1>@@RESULT@@</h1>
</BODY>
</HTML>
)=====";
