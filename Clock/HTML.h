const char CSS_html[] PROGMEM = R"=====(
<style>
body {
    font-family: Arial, sans-serif;
    margin: 0;
    padding: 20px;
    box-sizing: border-box;
}
.container {
    max-width: 600px;
    margin: 0 auto;
}
.control-group {
    margin-bottom: 20px;
}
.switch {
    position: relative;
    display: inline-block;
    width: 60px;
    height: 34px;
}
.switch input {
    display: none;
}
.slider {
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: #ccc;
    transition: .4s;
}
.slider:before {
    position: absolute;
    content: "";
    height: 26px;
    width: 26px;
    left: 4px;
    bottom: 4px;
    background-color: #fff;
    transition: .4s;
}
input:checked + .slider {
    background-color: #2196F3;
}
input:focus + .slider {
    box-shadow: 0 0 1px #2196F3;
}
input:checked + .slider:before {
    transform: translateX(26px);
}
.slider.round {
    border-radius: 34px;
}
.slider.round:before {
    border-radius: 50%;
}
input[type="range"] {
    width: 100%;
    max-width: 300px;
}
.alarm-group {
    margin-top: 20px;
}
.alarm-input {
    display: inline-block;
    margin-right: 10px;
    margin-bottom: 10px;
}
.alarm-input input[type="number"] {
    width: 60px;
}
@media (max-width: 600px) {
    body {
        padding: 10px;
    }
    .switch {
        width: 50px;
        height: 28px;
    }
    .slider:before {
        height: 22px;
        width: 22px;
        left: 3px;
        bottom: 3px;
    }
    input:checked + .slider:before {
        transform: translateX(22px);
    }
}
</style>
)=====";

const char header_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <META HTTP-EQUIV="CACHE-CONTROL" CONTENT="NO-CACHE">
    <title>Clock Settings</title>
</head>
<body>
<div class="container">
)=====";

const char footer_html[] PROGMEM = R"=====(
</div>
</body>
</html>
)=====";

const char radio_html_start[] PROGMEM = R"=====(
<div class="control-group">
    <h3>Display Mode</h3>
)=====";

const char radio_html_item[] PROGMEM = R"=====(
    <input type="radio" name="%s" value="%u" %s>%s<br />
)=====";

const char radio_html_end[] PROGMEM = R"=====(
</div>
)=====";

const char slider_html[] PROGMEM = R"=====(
<div class="control-group">
    <h3>Brightness</h3>
    <input id="%s" type="range" name="%s" value="%u" min="%u" max="%u" step="%u" /><br />
</div>
)=====";

const char button_html[] PROGMEM = R"=====(
<div class="control-group">
    <h3>Second Blink</h3>
    <label class="switch">
        <input type="checkbox" name="%s" >
        <span class="slider round"></span>
    </label>
    %s<br />
</div>
)=====";

const char alarm_html[] PROGMEM = R"=====(
<div class="control-group alarm-group">
    <h3>Set Alarm</h3>
    <div class="alarm-input">
        <label for="alarmHour">Hour:</label>
        <input type="number" id="alarmHour" name="alarmHour" min="1" max="12" value="%u">
    </div>
    <div class="alarm-input">
        <label for="alarmMinute">Minute:</label>
        <input type="number" id="alarmMinute" name="alarmMinute" min="0" max="59" value="%u">
    </div>
    <div class="alarm-input">
        <label for="alarmAmPm">AM/PM:</label>
        <select id="alarmAmPm" name="alarmAmPm">
            <option value="AM" %s>AM</option>
            <option value="PM" %s>PM</option>
        </select>
    </div>
    <div class="alarm-input">
        <label for="alarmEnabled">Enable Alarm:</label>
        <input type="checkbox" id="alarmEnabled" name="alarmEnabled" %s>
    </div>
</div>
)=====";

const char optAlarmHour[] PROGMEM = "alarmHour";
const char optAlarmMinute[] PROGMEM = "alarmMinute";
const char optAlarmAmPm[] PROGMEM = "alarmAmPm";
const char optAlarmEnabled[] PROGMEM = "alarmEnabled";


const char optDispMode[] PROGMEM = "Display Mode";
//const char optSB[] PROGMEM = "Second Blink";
const char optBRG[] PROGMEM = "Brightness";