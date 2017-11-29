const char ROOT_page[] PROGMEM = R"=====(
<HTML>
<TITLE>
Smart control: @@DEVNAME@@
</TITLE>
<BODY>
<h2>Device name: @@DEVNAME@@</h2>
Текущая дата: @@CURDATE@@
<table style="width:50%;border:double;margin:20px;">
<tr>
<th>Последняя дата</th><td>@@LDATE@@</td>
</tr>
<tr>
<th>Последнее значение температуры</th><td>@@LCELSIUM@@ &deg;C</td>
<th>Последнее значение влажности</th><td>@@LHUMIDITY@@%</td>
</tr>
</table>
<b><u>Доступые опции</u></b><br><br>

<a href="./setname">Смена имени устройства</a><br>
<a href="./setdate">Смена даты</a><br>
</BODY>
</HTML>
)=====";
