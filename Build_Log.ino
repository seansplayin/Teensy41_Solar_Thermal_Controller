/*
Teensy41_Solar_ThermalController_Copy_20260311014904 : Transitioned from LittleFS to SD
Teensy41_Solar_Thermal_Controller_copy_20260311070336 : Network Connected.
Teensy41_Solar_Thermal_Controller_copy_20260313225320 : Teensy is stable until attempting to connect to it's URL and then crashes but has occasionally been able to respond to the web connection before 2 blink crash
Teensy41_Solar_Thermal_Controller_copy_20260314111227 : Teensy is stable but not getting ip address
Teensy41_Solar_Thermal_Controller_copy_20260314122346 : Test webpages load, FirstWebpage crashes
Teensy41_Solar_Thermal_Controller_copy_20260315002852 : Implementing partial webpage 1/20th
Teensy41_Solar_Thermal_Controller_copy_20260317064655 : still in the middle of Implementing partial webpage 10/20th. Running into memeory troubles. added memory loging to firstwebpage.


Teensy41_Solar_Thermal_Controller_copy_20260314111227 : 
Teensy41_Solar_Thermal_Controller_copy_20260314111227 : 
Teensy41_Solar_Thermal_Controller_copy_20260314111227 : 
Teensy41_Solar_Thermal_Controller_copy_20260314111227 : 
Teensy41_Solar_Thermal_Controller_copy_20260314111227 : 








I checked the current ZIP you just uploaded, and here is the grounded picture from your actual codebase:

Teensy41_Solar_Thermal_Controller.ino still boots in HTTP isolation mode and does not call startAllTasks().

NetworkManager.cpp currently starts only:

/

/ping

setupFirstPageRoutes()

FirstWebpage.cpp now safely serves:

/firstpage-mini

/firstpage-midi

/firstpage-test as plain text

The original full page still depends on:

/static/favicon.png

/second-page

/third-page

/ws

WebSocket messages like hello:FirstWebpage, init, getUptime, ping, setAllPumps:auto, setPumpMode:*, setConfig:*, resetConfig, setTimeConfig:*, resetTimeConfig

Those backend pieces exist in your current code, but most are not registered in the active Teensy boot path yet:

serveStaticAssets()

serveFavicon()

initWebSocket()

setupRoutes()

setupAlarmRoutes()

setupSecondPageRoutes()

setupThirdPageRoutes()

SecondWebpage.cpp depends on /api/pump-runtimes, /list-logs, and /download-log.

ThirdWebpage.cpp depends on /hello, /temperature-logs/*, and /fs/*.

Your current compile environment already has the needed libraries resolved: SD, SdFat, SPI, freertos-teensy, RTClib, Adafruit BusIO, DallasTemperature, OneWire, ArduinoJson, AsyncWebServer_Teensy41, Teensy41_AsyncTCP, QNEthernet, Adafruit MAX31865, Wire, and Time.

The most important conclusion is this:

Do not bring firstPageHtml back directly

Your current tests proved:

firstpage-mini works

firstpage-midi works

the original monolithic page send crashes

So the rebuild needs to be dependency-first, not “cut the HTML into 20 chunks.”

Also, your current firstPageHtml contains duplicated JavaScript patterns, including a duplicated WebSocket bootstrap/onmessage area. So the rebuild should not be a blind transplant.

I cannot honestly guarantee a complex 20-step reintegration will succeed on the first pass, but this is the safest, most grounded order based on the current code you uploaded.






20-step staged rebuild plan for buildFirstPageMidiHtml()
1. Freeze the current known-good baseline

Keep these exactly as they are before every new step:

/ → root-ok

/ping → ok

/firstpage-mini

/firstpage-midi

/firstpage-test disabled as plain text

Do not re-enable firstPageHtml during this plan.

2. Stop using processor() for the new Teensy page

For the rebuilt page, inject VERSION_INFO and millis() directly in the builder functions, exactly like your current mini/midi routes do now.

Reason: the large-page request->send(..., processor) path is the thing that faults.

3. Split buildFirstPageMidiHtml() into internal helper builders

Before adding more UI, refactor the builder into small functions inside FirstWebpage.cpp, for example:

buildFirstPageHead()

buildFirstPageHeaderRow()

buildFirstPageTempsSection()

buildFirstPagePumpSection()

buildFirstPageConfigSection()

buildFirstPageFooterScripts()

This is not just for neatness. It gives you one revert point per section instead of one 60 KB failure blob.

4. Restore the original page head and CSS only

Copy only the <head> metadata and CSS from firstPageHtml into the new helper builders.

Include:

title

viewport

the current CSS rules

no JavaScript yet

keep the favicon line out until static routes are active

Pass condition:

/firstpage-midi still loads

no crash

styling appears

5. Restore the top 3-cell header row markup only

Rebuild the first table row from the original page:

time/date/uptime cell

“Solar Thermal System Controller” center cell

alarm/version/memory/filesystem right cell

Keep all values static placeholders for now.

Do not add editor logic or WebSocket code yet.

Pass condition:

layout renders

no JS

no crash

6. Restore the “System Temperatures” section markup only

Add the full temperature markup from the original page, preserving the original DOM ids like:

outsideT

storageT

panelT

CSupplyT

CreturnT

supplyT

CircReturnT

and the sensor ids such as DTemp1, DTempAverage1, etc.

Reason: later WebSocket updates rely on these exact ids.

Pass condition:

page still loads

temperatures show placeholder values only

no crash

7. Restore the heating call + pump-control section markup only

Add the markup for:

dhwHeatingCallStatus

heatingCallStatus

the pumps container

allAutoButton

allOffButton

Do not wire any click handlers yet.

Pass condition:

controls render

no actions yet

no crash

8. Restore the full configuration display/editor markup only

Add the entire Auto Pump Configuration section from the original page, including:

all display spans

all hidden inputs

all config buttons

both freeze sensor checkbox groups

Keep all ids exactly the same as the original page.

Reason: your existing JS and WebSocket config payloads are keyed to these ids and names.

Pass condition:

section renders

edit inputs exist

no handlers yet

no crash

9. Restore the full time configuration display/editor markup only

Add the time config UI from the original page:

timeInfoView

timeConfigEditor

timeZoneDisplay

dstEnabledDisplay

timeZoneSelect

dstEnabledSelect

the save/cancel/reset buttons

Again, markup only.

Pass condition:

section renders

no handlers yet

no crash

10. Restore the alarm button and iframe containers only

Add back:

alarmLogBtn

pumpRuntimesContainer + pumpRuntimesIframe

tempLogsContainer + tempLogsIframe

But at this step, leave iframe src blank or use placeholder text in the container. Do not point them at real routes yet.

Pass condition:

layout renders

no fetches/iframes yet

no crash

11. Activate the missing HTTP support routes in NetworkManager.cpp

Now enable the server-side routes your rebuilt page will need.

In the active startHttpServerOnce() path, add:

serveStaticAssets(server)

serveFavicon(server)

setupAlarmRoutes()

setupSecondPageRoutes()

setupThirdPageRoutes()

setupRoutes()

Why this exact set:

serveStaticAssets() is needed for /static/favicon.png

setupAlarmRoutes() is needed for the alarm log button

setupSecondPageRoutes() is needed for /second-page

setupThirdPageRoutes() is needed for /third-page

setupRoutes() is needed because SecondWebpage.cpp and ThirdWebpage.cpp also call:

/api/pump-runtimes

/list-logs

/download-log

/fs-stats

/get-log-data

/hello

Pass condition:

direct browser tests for /alarm-log, /second-page, /third-page, /hello, /fs-stats no longer 404

12. Turn on the real favicon and iframe src values

After Step 11 passes, put the original values back into the rebuilt page:

/static/favicon.png

/second-page?ts=<millis>

/third-page?ts=<millis>

Do not add iframe-scaling JS yet.

Pass condition:

iframes load

no crash

page still stable

13. Restore only the non-WebSocket JavaScript helpers

Now copy back only the JS that does not require /ws, specifically:

window.addEventListener("message", ...) for third-page height

setupAutoScaledIframe(...)

the DOMContentLoaded scaffolding around iframe measurement

alarm log button open handler

any purely local helper functions that do not send messages

Do not reintroduce any WebSocket code yet.

Pass condition:

iframes auto-scale

no /ws connection attempt

no crash

14. Restore local-only UI mode toggles

Bring back the JS for:

config edit/view mode

time config edit/view mode

local clock formatting helpers

local uptime formatting helpers

But do not start WebSocket yet.

Reason: these are safe and help prove your DOM ids are complete.

Pass condition:

edit buttons show/hide the correct blocks

still no WebSocket

no crash

15. Enable the WebSocket backend only

Now, in the active Teensy boot path, add initWebSocket().

Do not re-enable all tasks yet.

This step only gives you:

/ws

server.addHandler(&ws)

the existing hello/init/getUptime handling in WebServerManager.cpp

Pass condition:

browser can open /ws

no crash on page load

serial shows websocket connect events

16. Restore a single WebSocket bootstrap block in the page

Bring back only one clean WebSocket bootstrap in the page JS:

new WebSocket('ws://' + window.location.hostname + '/ws')

send:

hello:FirstWebpage

init

getUptime

handle reconnect

Important: do not copy both WebSocket blocks from the original firstPageHtml.
Your current raw literal contains duplicated WS setup logic. Keep only one.

Pass condition:

page opens

websocket connects

no crash

17. Restore read-only WebSocket message handlers first

Bring back handleWebSocketMessage(data) in this order:

DateTime:

Uptime:

SysStats:

FSStats:

AlarmState:

PumpStatus:

HeatingCalls:

Configuration:

TimeConfig:

Temperatures:

This order matches what your current backend already knows how to send from WebServerManager.cpp.

Pass condition:

values populate into existing DOM ids

no user actions yet

no crash

18. Re-enable the write-side page actions

Now restore the front-end actions that send existing backend commands:

setAllPumps:auto

setAllPumps:off

setPumpMode:*

setConfig:*

resetConfig

setTimeConfig:*

resetTimeConfig

Do not change the message names; they already match your current WebServerManager.cpp.

Pass condition:

button clicks send messages

no crash

serial shows expected backend handling

19. Re-enable only the minimum tasks required for live page updates

Do not immediately jump to full startAllTasks().

Bring tasks back in this order:

TaskWebSocketTransmitter

TaskUpdateTemperatures

TaskPumpControl

Why this order:

TaskWebSocketTransmitter gives you periodic stats and flagged outbound messages

TaskUpdateTemperatures makes temperature values live

TaskPumpControl makes pump/heating state meaningful

Important:
your current MainCtrl loop already calls serviceNetwork() continuously.
So do not add TaskNetworkPump until you intentionally decide whether you want:

network pumping in serviceNetwork()
or

network pumping in TaskNetworkPump

Do not run both casually.

Pass condition:

page stays up

websocket updates become live

temperatures/pump status begin changing

20. Re-enable storage/log/alarm features last

Only after the first page is stable with live data should you bring back the heavier subsystems:

AlarmManager_begin()

TaskLogger

TaskTemperatureLogging

AlarmHistory_begin()

Then fully validate the embedded pages and actions that depend on them:

/alarm-log

/second-page

/third-page

/api/pump-runtimes

/list-logs

/download-log

/temperature-logs/sensors

/temperature-logs/graph

/fs/*

This final step is where you reintroduce the parts that actually touch:

filesystem

log aggregation

temp logs

alarm history

pump runtime files

Only after Step 20 is stable should you consider retiring the old firstPageHtml completely and renaming the rebuilt midi page back to the real root page.

The key rules for this rebuild

Never put the big raw firstPageHtml back on the wire during the rebuild.

Never reintroduce WebSocket and task changes in the same step.

Never add a front-end feature before its backend route or message handler is active.

Keep only one WebSocket bootstrap block when you port the original JS.

Keep every original DOM id unchanged when you transplant markup, because your existing JS and backend payloads already target those ids.

If you want, I’ll turn this into Step 1 only with exact copy/paste edits against your current FirstWebpage.cpp, NetworkManager.cpp, and WebServerManager.h/cpp blocks.




Step 1 — replace the current buildFirstPageMidiHtml() with a modular scaffold
File

FirstWebpage.cpp

Replace this exact existing block

Start at:

static String buildFirstPageMidiHtml() {

End immediately before:

String processor(const String& var) {
With this exact replacement block
static String buildMidiHeadAndOpenTable() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>FirstPage Midi</title>
  <style>
    body{font-family:Arial,sans-serif;margin:12px;color:#222;}
    table{width:100%;table-layout:fixed;border-collapse:separate;border-spacing:5px;}
    td{vertical-align:top;min-width:320px;border:1px solid #999;padding:10px;}
    h1{color:purple;margin:0 0 8px 0;}
    h3{color:#459;margin:0 0 8px 0;}
    .blue-button{display:inline-block;padding:6px 10px;border:1px solid blue;color:blue;text-decoration:none;border-radius:4px;background:white;}
    .scaledFrame iframe{width:100%;border:0;min-height:200px;}
  </style>
</head>
<body>
<table border="0" cellpadding="4" cellspacing="5" bgcolor="white">
)rawliteral";
}

static String buildMidiRow1Col1() {
  return R"rawliteral(
<td valign="top">
  <h3>Row 1 Col 1</h3>
  <p>Step 1 scaffold placeholder</p>
</td>
)rawliteral";
}

static String buildMidiRow1Col2() {
  return R"rawliteral(
<td valign="top">
  <h1>Solar Control System</h1>
  <p>Step 1 scaffold placeholder</p>
</td>
)rawliteral";
}

static String buildMidiRow1Col3() {
  return R"rawliteral(
<td valign="top">
  <h3>Row 1 Col 3</h3>
  <p>Alarm / version / memory placeholder</p>
</td>
)rawliteral";
}

static String buildMidiRow2Col1() {
  return R"rawliteral(
<td valign="top">
  <h3>Row 2 Col 1</h3>
  <p>Temperature summary placeholder</p>
</td>
)rawliteral";
}

static String buildMidiRow2Col2() {
  return R"rawliteral(
<td valign="top">
  <h3>Row 2 Col 2</h3>
  <p>Pump control placeholder</p>
</td>
)rawliteral";
}

static String buildMidiRow2Col3() {
  return R"rawliteral(
<td valign="top">
  <div class="scaledFrame" id="pumpRuntimesContainer">
    <iframe id="pumpRuntimesIframe" scrolling="no"></iframe>
  </div>
</td>
)rawliteral";
}

static String buildMidiRow3Col1() {
  return R"rawliteral(
<td valign="top">
  <h3>Row 3 Col 1</h3>
  <p>Raw temperature list placeholder</p>
</td>
)rawliteral";
}

static String buildMidiRow3Col2() {
  return R"rawliteral(
<td valign="top">
  <h3>Row 3 Col 2</h3>
  <p>Configuration placeholder</p>
</td>
)rawliteral";
}

static String buildMidiRow3Col3() {
  return R"rawliteral(
<td valign="top">
  <div class="scaledFrame" id="tempLogsContainer">
    <iframe id="tempLogsIframe" scrolling="no"></iframe>
  </div>
</td>
)rawliteral";
}

static String buildMidiCloseTable() {
  return R"rawliteral(
</table>
)rawliteral";
}

static String buildMidiScripts() {
  return String();
}

static String buildMidiClosePage() {
  return R"rawliteral(
</body>
</html>
)rawliteral";
}

static String buildFirstPageMidiHtml() {
  String html;
  html.reserve(30000);

  html += buildMidiHeadAndOpenTable();

  html += "<tr>";
  html += buildMidiRow1Col1();
  html += buildMidiRow1Col2();
  html += buildMidiRow1Col3();
  html += "</tr>";

  html += "<tr>";
  html += buildMidiRow2Col1();
  html += buildMidiRow2Col2();
  html += buildMidiRow2Col3();
  html += "</tr>";

  html += "<tr>";
  html += buildMidiRow3Col1();
  html += buildMidiRow3Col2();
  html += buildMidiRow3Col3();
  html += "</tr>";

  html += buildMidiCloseTable();
  html += buildMidiScripts();
  html += buildMidiClosePage();

  return html;
}
Test after Step 1

Open:

http://10.20.90.39/ping

http://10.20.90.39/firstpage-midi

Both should still load without a 2-blink fault.

Step 2 — transplant the original page head/CSS into the new helper
File

FirstWebpage.cpp

Destination helper

Replace the body of:

static String buildMidiHeadAndOpenTable()
Exact source to copy from the current same file

Copy the exact current block from inside firstPageHtml starting at:

<!DOCTYPE html>
<html>
<head>

and ending at the line just before this exact marker:

<td valign="top" align="left" bgcolor="white" id="timeCell">

That copied block must include:

<link rel="icon" ...>

<title>

<meta ...>

the full <style type="text/css"> ... </style>

<body>

<table border="10" cellpadding="4" cellspacing="5" bgcolor="white">

the opening <tr>

Destination wrapper

Paste that block into:

static String buildMidiHeadAndOpenTable() {
  return R"rawliteral(
  ...paste copied block here...
  )rawliteral";
}
Test after Step 2

Load /firstpage-midi.
It should still load and now use the original page CSS.

Step 3 — transplant original top-left time/status cell
File

FirstWebpage.cpp

Destination helper

Replace the body of:

static String buildMidiRow1Col1()
Exact source to copy

Copy the exact current block from firstPageHtml starting at:

<td valign="top" align="left" bgcolor="white" id="timeCell">

and ending at its matching closing:

</td>

Stop before the next exact marker:

<td valign="top" align="center" bgcolor="white">
Paste format
static String buildMidiRow1Col1() {
  return R"rawliteral(
  ...paste exact copied block here...
  )rawliteral";
}
Test

Load /firstpage-midi.
The left time/date/time-config cell should render without JS yet.

Step 4 — transplant original top-center title cell
File

FirstWebpage.cpp

Destination helper

Replace:

static String buildMidiRow1Col2()
Exact source to copy

Copy the exact current block from firstPageHtml starting at:

<td valign="top" align="center" bgcolor="white">

and ending at its matching </td>

Stop before:

<td valign="top" align="center" bgcolor="white" id="statusCell">
Test

Load /firstpage-midi.
The “Solar Thermal / System Controller” title cell should render.

Step 5 — transplant original top-right alarm/version/memory cell
File

FirstWebpage.cpp

Destination helper

Replace:

static String buildMidiRow1Col3()
Exact source to copy

Copy the exact current block from firstPageHtml starting at:

<td valign="top" align="center" bgcolor="white" id="statusCell">

and ending at its matching </td>

Stop before the next </tr>.

Test

Load /firstpage-midi.
You should now have the original full top row.

Step 6 — transplant original row 2 col 1 “System Temperatures” section
File

FirstWebpage.cpp

Destination helper

Replace:

static String buildMidiRow2Col1()
Exact source to copy

Copy the exact current block from firstPageHtml starting at:

<td valign="top" id="configCell">
      <div id="SectionHeader" class="configContent">      
        <h3>System Temperatures</h3>

and ending at the matching </td> for that column.

Stop before the next exact marker:

<td valign="top" id="configCell">
      <div id="SectionHeader" class="configContent">
        <h3>Relay Status & Control</h3>
Test

Load /firstpage-midi.
The temperature summary column should appear with all original ids intact.

Step 7 — transplant original row 2 col 2 relay/pump control section
File

FirstWebpage.cpp

Destination helper

Replace:

static String buildMidiRow2Col2()
Exact source to copy

Copy the exact current block from firstPageHtml starting at:

<td valign="top" id="configCell">
      <div id="SectionHeader" class="configContent">
        <h3>Relay Status & Control</h3>

and ending at its matching </td>

Stop before the next exact marker:

<td valign="top" bgcolor="white" align="center">
        <div class="scaledFrame" id="pumpRuntimesContainer">
Test

Load /firstpage-midi.
The pump-control column should appear, still without live actions.

Step 8 — transplant original row 2 col 3 pump runtime iframe section
File

FirstWebpage.cpp

Destination helper

Replace:

static String buildMidiRow2Col3()
Exact source to copy

Copy the exact current block from firstPageHtml starting at:

<td valign="top" bgcolor="white" align="center">
        <div class="scaledFrame" id="pumpRuntimesContainer">

and ending at its matching </td>

Do not change the iframe id:

id="pumpRuntimesIframe"
Test

Load /firstpage-midi.
The pump runtime iframe container should render, even though it will be blank until routes are activated.

Step 9 — transplant original row 3 col 1 raw temperature list
File

FirstWebpage.cpp

Destination helper

Replace:

static String buildMidiRow3Col1()
Exact source to copy

Copy the exact current block from firstPageHtml starting at:

<td valign="top" id="configCell">
        <div id="SectionHeader">
          <h3>Temperature Values</h3>

and ending at its matching </td>

Stop before the next exact marker:

<td valign="top" id="configCell">
        <div id="SectionHeader" class="configContent">
          <h3>Auto Pump Configuration</h3>
Test

Load /firstpage-midi.
The raw temperature list should appear.

Step 10 — transplant original row 3 col 2 configuration editor
File

FirstWebpage.cpp

Destination helper

Replace:

static String buildMidiRow3Col2()
Exact source to copy

Copy the exact current block from firstPageHtml starting at:

<td valign="top" id="configCell">
        <div id="SectionHeader" class="configContent">
          <h3>Auto Pump Configuration</h3>

and ending at its matching </td>

Stop before:

<td valign="top" bgcolor="white" align="center">
        <div class="scaledFrame" id="tempLogsContainer">
Test

Load /firstpage-midi.
The full configuration area should appear.

Step 11 — transplant original row 3 col 3 temp logs iframe section
File

FirstWebpage.cpp

Destination helper

Replace:

static String buildMidiRow3Col3()
Exact source to copy

Copy the exact current block from firstPageHtml starting at:

<td valign="top" bgcolor="white" align="center">
        <div class="scaledFrame" id="tempLogsContainer">

and ending at its matching </td>

Test

Load /firstpage-midi.
The full static page layout is now back, but still without routes/scripts/live data.

Step 12 — activate the supporting non-WebSocket routes
Files

WebServerManager.h
NetworkManager.cpp

12A — WebServerManager.h

Add these exact prototypes under:

String getContentType(const String& path);

Paste:

void serveStaticAssets(AsyncWebServer& server);
void serveFavicon(AsyncWebServer& server);
void setupRoutes();
12B — NetworkManager.cpp

Add these includes near the top with the others:

#include "SecondWebpage.h"
#include "ThirdWebpage.h"
12C — replace the body of startHttpServerOnce()

Replace the current function body with this exact order inside it:

  // Minimal root test route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[HTTP] GET /");
    request->send(200, "text/plain", "root-ok");
  });

  // Minimal ping test route
  server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[HTTP] GET /ping");
    request->send(200, "text/plain", "ok");
  });

  serveStaticAssets(server);
  serveFavicon(server);

  setupAlarmRoutes();
  setupSecondPageRoutes();
  setupThirdPageRoutes();
  setupRoutes();
  setupFirstPageRoutes();

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.print("[HTTP] 404 ");
    Serial.println(request->url());
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  s_serverStarted = true;
  Serial.println("[Network] AsyncWebServer started on port 80");
Test after Step 12

Open these directly:

/alarm-log

/second-page

/third-page

/hello?from=test

/fs-stats

They should no longer 404.

Step 13 — add only the non-WebSocket iframe/alarm script
File

FirstWebpage.cpp

Destination helper

Replace:

static String buildMidiScripts() {
  return String();
}
With a raw-literal script shell

Use:

static String buildMidiScripts() {
  return R"rawliteral(
<script>
window.addEventListener("message", (event) => {
  if (!event.data || event.data.type !== "thirdPageHeight") return;
  if (event.origin !== window.location.origin) return;
  const iframe = document.getElementById("tempLogsIframe");
  if (!iframe) return;
  iframe.style.height = (event.data.height + 10) + "px";
});

document.addEventListener('DOMContentLoaded', function () {
  var ws = null;

  const alarmBtn = document.getElementById('alarmLogBtn');
  if (alarmBtn) {
    alarmBtn.addEventListener('click', () => {
      const w = window.open('/alarm-log', '_blank');
      if (w) w.opener = null;
    });
  }

  function setupAutoScaledIframe(containerId, iframeId, maxScale) {
    // paste the exact current block from firstPageHtml
  }

  setupAutoScaledIframe('pumpRuntimesContainer', 'pumpRuntimesIframe', 2);
  setupAutoScaledIframe('tempLogsContainer', 'tempLogsIframe', 2);
});
</script>
)rawliteral";
}
Exact source blocks to paste into that shell

From the current firstPageHtml script, copy exactly:

the current block starting:

function setupAutoScaledIframe(containerId, iframeId, maxScale) {

and ending at its matching }

keep the window.addEventListener("message"... block exactly as it already exists now

keep the current alarmBtn handler exactly as it already exists now

Test

Load /firstpage-midi.
The page should load, alarm button should open /alarm-log, and iframes should resize once their pages exist.

Step 14 — add the config editor local-only logic
File

FirstWebpage.cpp

Destination

Inside the same buildMidiScripts() DOMContentLoaded block

Exact source to copy from current firstPageHtml

Copy the exact current block starting at:

let currentConfig = {};

and ending immediately before the exact comment:

// ------------- Time config edit/cancel/save UI -------------

That copied block already contains:

currentConfig

configEditMode

currentCollectorSensors

currentLineSensors

configUnits

configKeys

sensorNames

formatFreezeSensorLines()

the config button bindings

setConfigEditMode()

Important

Do not alter the ids it references.

Test

Load /firstpage-midi.
The config edit buttons should switch the UI into edit mode without crashing.
Saving will still not work until the WebSocket steps are active.

Step 15 — add the time configuration local-only logic
File

FirstWebpage.cpp

Destination

Inside buildMidiScripts() DOMContentLoaded

Exact source to copy

Copy the exact current block starting at this exact comment:

// ------------- Time config edit/cancel/save UI -------------

and ending at:

updateTimeConfigView();

Also make sure the earlier declaration:

let timeConfig = {
  timeZoneId: 'US_MOUNTAIN',
  dstEnabled: 1
};

is present before this block.

Test

Load /firstpage-midi.
Time config view/edit should toggle correctly without crashing.

Step 16 — enable the WebSocket backend only
File

NetworkManager.cpp

Exact edit

Inside startHttpServerOnce(), add this line:

initWebSocket();

Place it after:

setupRoutes();

and before:

setupFirstPageRoutes();

So the order becomes:

  setupAlarmRoutes();
  setupSecondPageRoutes();
  setupThirdPageRoutes();
  setupRoutes();
  initWebSocket();
  setupFirstPageRoutes();
Test

After reboot, load /firstpage-midi.
Nothing visual should change yet, but the /ws endpoint is now active.

Step 17 — add a single clean WebSocket bootstrap to the page
File

FirstWebpage.cpp

Destination

Inside buildMidiScripts() DOMContentLoaded block

Exact source to copy

From the current firstPageHtml script, copy exactly:

from:

var ws = null;
var wsReconnectTimer = null;
var wsBackoffMs = 1000;
var wsBackoffMaxMs = 30000;

through:

wsConnect();
Important

Do not copy the later duplicate block that starts with:

ws.onopen = function () {

That later block is the duplicate one in your current monolithic page and must stay out of the rebuild.

Test

Load /firstpage-midi and confirm it still does not hard-fault.

Step 18 — add the read-only message handling and local ticking
File

FirstWebpage.cpp

Destination

Inside buildMidiScripts() DOMContentLoaded block, after Step 17

Exact source to copy

Copy these exact current blocks from firstPageHtml:

from:

const pumpStates = Array(11).fill(null);

through:

function formatFreezeSensorLines(arr) {

including:

the 1..10 initialization loop

uptime tick state

clock tick state

parseUptimeToSeconds

formatUptimeFromSeconds

startUptimeTickerFromString

startClockTicker

configUnits

configKeys

sensorNames

then copy the exact current block:

function handleWebSocketMessage(data) {

through the end of the read-only cases:

Temperatures:

Configuration:

TimeConfig:

DateTime:

bare date-time fallback

Uptime:

SysStats:

Heap:

PSRAM:

FSStats:

PumpStatus:

legacy single pump update

HeatingCalls:

AlarmState:

Stop at the line:

} // end handleWebSocketMessage
Test

Load /firstpage-midi.
The page should still load without a 2-blink fault.

Step 19 — add the write-side UI actions and pump UI builder
File

FirstWebpage.cpp

Destination

Inside buildMidiScripts() DOMContentLoaded block, after Step 18

Exact source to copy

Copy these exact current blocks from firstPageHtml:

the current block starting at:

function updatePumpStatuses(pumpStatusData) {

through the end of:

window.changePumpMode = changePumpMode;

the current block that binds:

document.getElementById('allAutoButton').addEventListener('click', ...
document.getElementById('allOffButton').addEventListener('click', ...

the current save/reset/write handlers that send:

setConfig:...

resetConfig

setTimeConfig:...

resetTimeConfig

setPumpMode:...

Important

By Step 19, keep only one WebSocket bootstrap and one handleWebSocketMessage() function. Do not bring back the duplicate ws.onopen / ws.onmessage block from the monolithic page.

Test

Page loads. Buttons now send real WebSocket messages.

Step 20 — re-enable only the UI/live-data tasks, not the full old task set
Files

TaskManager.h
TaskManager.cpp
Teensy41_Solar_Thermal_Controller.ino

20A — TaskManager.h

Add this exact prototype under:

void startAllTasks();

Paste:

void startUiTasksOnly();
20B — TaskManager.cpp

Add this exact function after startAllTasks():

void startUiTasksOnly() {
  Serial.println("[StartUiTasksOnly] Entered");

  Serial.println("[StartUiTasksOnly] Calling AlarmManager_begin()");
  AlarmManager_begin();
  Serial.println("[StartUiTasksOnly] AlarmManager_begin() done");

  Serial.println("[StartUiTasksOnly] Creating TaskPumpControl");
  xTaskCreate(TaskPumpControl, "PumpControl", 4096, NULL, 4, &thPumpControl);

  Serial.println("[StartUiTasksOnly] Creating TaskUpdateTemperatures");
  xTaskCreate(TaskUpdateTemperatures, "UpdateTemps", 4096, NULL, 3, &thUpdateTemps);

  Serial.println("[StartUiTasksOnly] Creating TaskWebSocketTransmitter");
  xTaskCreate(TaskWebSocketTransmitter, "WsTx", 4096, NULL, 2, NULL);

  Serial.println("[StartUiTasksOnly] Exit");
}
20C — Teensy41_Solar_Thermal_Controller.ino

Replace this exact existing block:

  // 4. Start Tasks
  Serial.println("[System] Skipping Application Tasks for HTTP isolation test");
  for (;;) {
      serviceNetwork();
    vTaskDelay(pdMS_TO_TICKS(1));
  }

with this exact block:

  // 4. Start UI / live-data tasks only
  Serial.println("[System] Starting UI tasks only");
  startUiTasksOnly();

  for (;;) {
    serviceNetwork();
    vTaskDelay(pdMS_TO_TICKS(1));
  }

*/