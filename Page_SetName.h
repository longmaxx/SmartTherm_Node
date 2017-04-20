const char SETNAME_page[] PROGMEM = R"=====(
<HTML>
<TITLE>
Name Settings
</TITLE>
<BODY>
<FORM method="post">
Set new device name:<br>
<INPUT TYPE=TEXT value="@@DEVNAME@@" name=devname>
<INPUT TYPE=SUBMIT name=submit>
</FORM>
</BODY>
</HTML>
)=====";
