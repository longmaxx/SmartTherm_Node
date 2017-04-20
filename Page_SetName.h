const char SETNAME_page[] PROGMEM = R"=====(
<HTML>
<TITLE>
Name Settings
</TITLE>
<BODY>
<a href="./">Главная</a>>Смена имени устройства<br>
<FORM method="post">
<INPUT TYPE=TEXT value="@@DEVNAME@@" name=devname>
<INPUT TYPE=SUBMIT name=submit>
</FORM>
<h1>@@RESULT@@</h1>
</BODY>
</HTML>
)=====";
