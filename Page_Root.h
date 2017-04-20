const char ROOT_page[] PROGMEM = R"=====(
<HTML>
<TITLE>
Smart control: @@DEVNAME@@
</TITLE>
<BODY>
Последняя дата: @@CURDATE@@<br><br>

Последняя дата: @@LDATE@@<br>
Последняя температура: @@LCELSIUM@@<br><br>

<b><u>Доступые опции</u></b><br><br>

<a href="./setname">Смена имени устройства</a><br>
<a href="./setdate">Смена даты</a><br>
</BODY>
</HTML>
)=====";
