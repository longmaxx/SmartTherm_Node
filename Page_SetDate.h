const char SETDATE_page[] PROGMEM = R"=====(
<HTML>
<TITLE>
Date</TITLE>
<BODY>
<a href="./">Главная</a>>Смена даты<br>
<FORM method="post">
<INPUT TYPE=number value=2017 min=2000 max=2100  name=year>
<INPUT TYPE=number  min=1  max=12  name=month>
<INPUT TYPE=number  min=1 max=31 name=day>
<INPUT TYPE=number  min=0 max=23 name=hour>
<INPUT TYPE=number min=0 max=59  name=minute>
<INPUT TYPE=SUBMIT name=submit>
</FORM>
<h1>@@RESULT@@</h1>
</BODY>
</HTML>
)=====";
