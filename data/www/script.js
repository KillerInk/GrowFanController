var slider = document.getElementById("speed");
var output = document.getElementById("sliderAmount");
var slider1 = document.getElementById("speed1");
var output1 = document.getElementById("sliderAmount1");
var button0 = document.getElementById("fan0sub");
var button1 = document.getElementById("fan1sub");
var fan0min = document.getElementById("fan0min");
var fan1min = document.getElementById("fan1min");
var fan0max = document.getElementById("fan0max");
var fan1max = document.getElementById("fan1max");
var temperatur = document.getElementById("temperatur");
var humidity = document.getElementById("humidity");
var temperatur_g = document.getElementById("temperatur_govee");
var humidity_g = document.getElementById("humidity_govee");
var battery = document.getElementById("battery");
var buttonSubTempHum = document.getElementById("targetTempHumSub")
var targettemp = document.getElementById("targettemp");
var targethum = document.getElementById("targethum");
var enableauto = document.getElementById("enableauto");
var aqi = document.getElementById("aqi");
var eco2 = document.getElementById("eco2");
var tvoc = document.getElementById("tvoc");
var speeddif = document.getElementById("speeddif");
var readgovee = document.getElementById("enablereadgovee");

var autofan0 = document.getElementById("autofan0");
var autofan1 = document.getElementById("autofan1");

var targetTempHumDiffSub = document.getElementById("targetTempHumDiffSub");
var tempdif = document.getElementById("tempdif");
var humdif = document.getElementById("humdif");

var minspeed = document.getElementById("minspeed");
var maxspeed = document.getElementById("maxspeed");
var minmaxspeedsub = document.getElementById("minmaxspeedsub");

output.innerHTML = slider.value;
output1.innerHTML = slider1.value;
var websocket;


function createWebsocket() {
  websocket = new WebSocket('ws://' + document.location.host + ":/ws");

  websocket.onmessage = function (data) {
    var result = JSON.parse(data.data); // $.parseJSON(data);
    var bat = result["battery"];
    var temp = result["temperatur"];
    var hum = result["humidity"];
    var atemp = result["atemperatur"];
    var ahum = result["ahumidity"];
    var autospeed = result["autocontrolspeed"];
    var ec2 = result["eco2"];
    var tvc = result["tvoc"];
    var aq = result["aqi"];
    var volt0 = result["voltage0"];
    var volt1 = result["voltage1"];
    var nightmodeactive = result["nightmode"];
    if (bat != undefined) {
      battery.innerHTML = bat + "%";
      temperatur_g.innerHTML = temp + "&deg;C";
      humidity_g.innerHTML = hum + "%";
    }
    else {
      temperatur.innerHTML = temp + "&deg;C A:" + atemp + "&deg;C";
      humidity.innerHTML = hum + "% A:" + ahum + "%";
    }
    if (autospeed != undefined) {
      autofan0.innerHTML = "Fan1:" + autospeed + "% " + volt0 + "mv ";
      autofan1.innerHTML = "Fan2:" + (autospeed - speeddif.value) + "% " + volt1 + "mv";
    }
    if (ec2 != undefined)
      eco2.innerHTML = "eCO2:" + ec2 + "ppm";
    if (tvc != undefined)
      tvoc.innerHTML = "TVOC:" + tvc + "ppb";
    if (aq != undefined) {
      aqi.innerHTML = "Aqi: " + aq;
    }
    document.getElementById("nightmodeactive").innerHTML = nightmodeactive;

  };

  websocket.onerror = function (error) {
    createWebsocket();
  };
}

window.addEventListener('load', (event) => {
  let host = document.location.origin;
  const query = `${host}/settings`;
  fetch(query)
    .then((response) => response.json())
    .then(json => {
      fan0min.value = json["fan0min"];
      fan0max.value = json["fan0max"];
      fan1min.value = json["fan1min"];
      fan1max.value = json["fan1max"];
      targettemp.value = json["targetTemperature"];
      targethum.value = json["targetHumidity"];
      enableauto.checked = json["autocontrol"];
      readgovee.checked = json["readgovee"];
      speeddif.value = json["speeddif"];
      tempdif.value = json["tempdif"]
      humdif.value = json["humdif"]
      minspeed.value = json["minspeed"];
      maxspeed.value = json["maxspeed"];
      document.getElementById("onhour").value = json["nightmodeonhour"];
      document.getElementById("onmin").value = json["nightmodeonmin"];
      document.getElementById("offmin").value = json["nightmodeoffmin"];
      document.getElementById("offhour").value = json["nightmodeoffhour"];
      document.getElementById("nightmodemaxspeed").value = json["nightmodemaxspeed"];
      document.getElementById("enablenightmode").checked = json["nightmodeactive"];
    });
  createWebsocket();
});

// Display the default slider value

// Update the current slider value (each time you drag the slider handle)
slider.oninput = function () {
  output.innerHTML = this.value;
  let host = document.location.origin;
  let value = encodeURIComponent(this.value);

  const query = `${host}/cmd?var=speed&val=${value}&id=0`;

  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

slider1.oninput = function () {
  output1.innerHTML = this.value;
  let host = document.location.origin;
  let value = encodeURIComponent(this.value);

  const query = `${host}/cmd?var=speed&val=${value}&id=1`;

  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

button0.onclick = function () {
  let host = document.location.origin;
  let min = document.getElementById("fan0min").value;
  let max = document.getElementById("fan0max").value;
  const query = `${host}/cmd?var=voltage&min=${min}&max=${max}&id=0`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

button1.onclick = function () {
  let host = document.location.origin;
  let min = document.getElementById("fan1min").value;
  let max = document.getElementById("fan1max").value;
  const query = `${host}/cmd?var=voltage&min=${min}&max=${max}&id=1`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

buttonSubTempHum.onclick = function () {
  let host = document.location.origin;
  let temp = targettemp.value;
  let hum = targethum.value;
  let speed = speeddif.value;
  const query = `${host}/cmd?var=autovals&temp=${temp}&hum=${hum}&speeddif=${speed}`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

targetTempHumDiffSub.onclick = function () {
  let host = document.location.origin;
  let temp = tempdif.value;
  let hum = humdif.value;
  const query = `${host}/cmd?var=temphumdif&temp=${temp}&hum=${hum}`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

enableauto.onclick = function () {
  let host = document.location.origin;
  let val = enableauto.checked;
  if (val == false)
    val = 0;
  else
    val = 1;
  const query = `${host}/cmd?var=autocontrol&val=${val}`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

readgovee.onclick = function () {
  let host = document.location.origin;
  let val = readgovee.checked;
  if (val == false)
    val = 0;
  else
    val = 1;
  const query = `${host}/cmd?var=readgovee&val=${val}`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

minmaxspeedsub.onclick = function () {
  let host = document.location.origin;
  let min = minspeed.value;
  let max = maxspeed.value;
  const query = `${host}/cmd?var=autospeed&min=${min}&max=${max}`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

var nightmodesub = document.getElementById("nightmodesub");

nightmodesub.onclick = function () {
  let host = document.location.origin;
  let onhour = document.getElementById("onhour").value;
  let onmin = document.getElementById("onmin").value;
  let offmin = document.getElementById("offmin").value;
  let offhour = document.getElementById("offhour").value;
  let mspeed = document.getElementById("nightmodemaxspeed").value;
  const query = `${host}/cmd?var=fannightmode&onh=${onhour}&onm=${onmin}&offh=${offhour}&offm=${offmin}&mspeed=${mspeed}`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

var enablenightmode = document.getElementById("enablenightmode");

enablenightmode.onclick = function () {
  let host = document.location.origin;
  let val = enablenightmode.checked;
  if (val == false)
    val = 0;
  else
    val = 1;
  const query = `${host}/cmd?var=fannightmodeactive&nighton=${val}`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}
