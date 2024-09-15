
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
    var nightmodeactive = result["nighmode"];
    var time = result["time"];
    var lightp = result["lightvalP"];
    var lightmv = result["lightvalmv"];
    var lightstate = result["lightstate"];
    var vpda = result["vpdair"];
    document.getElementById("time").innerHTML = time;
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
    if (ec2 != undefined) {
      eco2.innerHTML = "eCO2:" + ec2 + "ppm";
    }
    if (tvc != undefined)
      tvoc.innerHTML = "TVOC:" + tvc + "ppb";
    if (aq != undefined) {
      aqi.innerHTML = "Aqi: " + aq;
    }
    if (nightmodeactive != undefined)
      document.getElementById("nightmodeactive").innerHTML = nightmodeactive;
    if (lightp != undefined)
      document.getElementById("lightpercent").innerHTML = "Light % " + lightp;
    if (lightmv != undefined) {
      document.getElementById("lightmv").innerHTML = "Light mv " + lightmv;
    }
    if (lightstate != undefined) {
      var n;
      if (lightstate == 0)
        n = "off";
      else if (lightstate == 1)
        n = "on";
      else if (lightstate == 2)
        n = "sunrise";
      else if (lightstate == 3)
        n = "sunset";
      document.getElementById("lightstate").innerHTML = "Light state " + n;
    }
    addChartItems(time, atemp, ahum, autospeed, lightmv, ec2, vpda,true);
    if (timeVals.length - mychart.config.options.scales.x.max < 10) {
      let dif = mychart.config.options.scales.x.max - mychart.config.options.scales.x.min;
      mychart.config.options.scales.x.max = timeVals.length;
      mychart.config.options.scales.x.min = timeVals.length - dif;
      
      //mychart.config.options.scales.x.min = timeVals.length - 500;
    }
    mychart.update();
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

      document.getElementById("turnlightonhour").value = json["lightonh"];
      document.getElementById("turnlightonmin").value = json["lightonmin"];

      document.getElementById("turnlightoffhour").value = json["lightoffh"];
      document.getElementById("turnlightoffmin").value = json["lightoffmin"];

      document.getElementById("sunrisehour").value = json["lightriseh"];
      document.getElementById("sunrisemin").value = json["lightrisemin"];

      document.getElementById("sunsethour").value = json["lightseth"];
      document.getElementById("sunsetmin").value = json["lightsetmin"];

      document.getElementById("enablesunrise").checked = json["lightriseenable"];
      document.getElementById("enablesunset").checked = json["lightsetenable"];
      document.getElementById("enablelightcontrol").checked = json["lightautomode"];

      document.getElementById("lightminv").value = json["lightminvolt"];
      document.getElementById("lightmaxv").value = json["lightmaxvolt"];

    });
  getChartDataForToday();
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

var buttonsubmitlightvoltage = document.getElementById("lightcontrolvoltagesub");

buttonsubmitlightvoltage.onclick = function () {
  let host = document.location.origin;
  let min = document.getElementById("lightminv").value;
  let max = document.getElementById("lightmaxv").value;
  const query = `${host}/cmd?var=lightvoltage&min=${min}&max=${max}`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

var lightslider = document.getElementById("lightstrength");

lightslider.oninput = function () {
  let host = document.location.origin;
  let value = encodeURIComponent(this.value);
  const query = `${host}/cmd?var=lightval&val=${value}`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

var submitLightTimes = document.getElementById("lightcontrolsub");

submitLightTimes.onclick = function () {
  let host = document.location.origin;
  let onh = document.getElementById("turnlightonhour").value;
  let onmin = document.getElementById("turnlightonmin").value;
  let offh = document.getElementById("turnlightoffhour").value;
  let offmin = document.getElementById("turnlightoffmin").value;
  let riseh = document.getElementById("sunrisehour").value;
  let risemin = document.getElementById("sunrisemin").value;
  let seth = document.getElementById("sunsethour").value;
  let setmin = document.getElementById("sunsetmin").value;
  let enablerise = document.getElementById("enablesunrise").checked;
  if (enablerise == false)
    enablerise = 0;
  else
    enablerise = 1;
  let enableset = document.getElementById("enablesunset").checked;
  if (enableset == false)
    enableset = 0;
  else
    enableset = 1;
  const query = `${host}/cmd?var=lightsettime&onh=${onh}&onmin=${onmin}&offh=${offh}&offmin=${offmin}&riseh=${riseh}&risemin=${risemin}&seth=${seth}&setmin=${setmin}&riseenable=${enablerise}&setenable=${enableset}`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}

var enableautolight = document.getElementById("enablelightcontrol");

enableautolight.onclick = function () {
  let host = document.location.origin;
  let val = enableautolight.checked;
  if (val == false)
    val = 0;
  else
    val = 1;
  const query = `${host}/cmd?var=lightautomode&enable=${val}`;
  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
}



