#pragma once

constexpr auto WebPage =  std::string_view( R"html(

<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta charset='utf-8'><title>Hladinoměr</title>
</head>
<style>
body {
    margin: 0;
    padding: 0;
    background-color: #1e1c2c;
}

body, input, button {
    font-family:sans-serif;
    color: #fbf4e5;
    
}

input {
    border-width: 1px;
    background-color: black;
    font-size: 1.1rem;
}

button,input[type=submit] {
    background-color: #6f688a;
    font-size: 1.1rem;
}

h1 {text-align:center;}

.raw {
    font-size:0.8rem;
}

.form {
    position:relative;
    max-width: 18rem;
    margin:auto;
}

table.hl {
    width: 100%;
    font-size: 1.7rem;
    max-width: 18rem;
    margin: auto;
}

table.hl th {text-align:right;}
table.hl td:nth-child(2) {text-align:right;}
    
table.scan {
    border: 1px solid;
    border-collapse: collapse;
    margin-top: 1rem;
    width: 100%;    
}

table.scan th {
    background-color: #000;
    padding: 0.2rem;
    border-right: 1px dotted;
}

table.scan td {
    padding: 0.2rem;
    border-right: 1px dotted;
}

.form label {
    display:flex;
    position: relative;
    margin: 0.2rem;
    justify-content: space-between;
}

.form label > span {
    width: 44%;
    flex-grow: 0;
}
.form label > *:last-child {
    flex-grow:1;
    flex-shrink:1;
}

input[type=number] {
    width: 5rem;
    text-align: center;
}
.form label input[type=number] {
    flex-grow: 0;
}
input[type=text] {
    text-align:center;
    width: 5rem;
}
.form .buttons {
    text-align:right;
}
table.scan tr {
    cursor: pointer;
}
table.scan tr td:last-child {
    text-align:right;
}
    
</style>
<script type="text/javascript">

function all(name) {
    return document.getElementById(name);
}

function fetchData() {
    fetch("/data").then(x=>x.json()).then(data=>{
        var hladina = all("hladina");
        var trend = all("trend");
        var raw = all("raw");
        hladina.textContent = (data.level*0.001).toFixed(2);
        trend.textContent = (data.trend*0.001).toFixed(2);
        raw.textContent = data.raw;
    });
    setTimeout(fetchData,1000);
}

function start() {
    setTimeout(fetchData,1000);
}

function doConfigShow() {
    const p = this.parentElement;
    const nx1 = p.nextElementSibling;
    nx1.hidden = !nx1.hidden;
    fetch("/config").then(x=>x.json()).then(data=>{
        all("nullpoint").value = data.zeropt*0.1;
        all("trendsec").value = data.trend_sec;        
    });
    
}

function doScan() {
    const p = this.parentElement;
    const nx1 = p.nextElementSibling;
    nx1.hidden = !nx1.hidden;
    if (!nx1.hidden) {

        function runScan() {
            if (nx1.hidden) return ;
            fetch("/scan").then(x=>x.json())
                .then(data=>{
                    if (data.length == 0) return;
                    let sr = all("scan_result");
                    sr.innerHTML="";
                    data.sort((a,b)=>b.RSSI - a.RSSI).forEach(x=>{
                        let r = document.createElement("tr");
                        let d1 = document.createElement("td");
                        let d2 = document.createElement("td");
                        let perc =  2 * (x.RSSI + 100);
                        if (perc > 100) perc = 100;
                        if (perc < 1) perc = 1;
                        
                        d1.textContent = x.SSID;
                        d2.textContent = perc+" %";
                        r.appendChild(d1);
                        r.appendChild(d2);
                        sr.appendChild(r);
                        r.onclick = ()=>{
                            all("ssid").value = x.SSID;
                        };
                    });
                    setTimeout(runScan,5000);
                });
            
        }

        runScan();
    }
}


function savecfg() {
    const nullpt = all("nullpoint").valueAsNumber;
    const trendsec = all("trendsec").valueAsNumber;
    if (!isNaN(nullpt) && !isNaN(trendsec) ) {
        const f = new FormData();
        f.set("zeropt", nullpt*10);
        f.set("trend_sec", trendsec);
        fetch("/config", {
            "method":"POST",
            "body": f
        }).then(x=>{
            if (x.status == 202) {
                alert("Nastavení uloženo");
            } else {
                alert("Chyba při ukládání");
            }
        })
    } else {
        alert("Nevyplněné políčko");
    }
}

function savewifi() {
    const ssid = all("ssid").value
    const pass = all("pass").value;
    if (ssid.length> 0) {
        const f = new FormData();
        f.set("ssid", ssid);
        f.set("pass", pass);
        fetch("/savewifi", {
            "method":"POST",
            "body": f
        }).then(x=>{
            if (x.status == 202) {
                alert("Nastavení uloženo");
            } else {
                alert("Chyba při ukládání");
            }
        })
    } else {
        alert("Nevyplněné políčko");
    }
}



</script>
<body onload="start()">
<h1>Hladinoměr</h1>
<table class="hl">
<tr><th>Hladina:</th><td id="hladina"></td><td>m</td></tr>
<tr><th>Trend:</th><td id="trend"></td><td>m/h</td></tr>
<tr th class="raw"><th>(raw):</th><td id="raw"></td><td></td></tr>
</table>
<hr>
    <label>
        <input type="checkbox" onchange="doConfigShow.call(this)"> Kalibrace a nastavení
    </label>
    <div hidden="hidden" class="form">
        <label><span>Výška [cm]</span>
            <input type="number" id="nullpoint">
        </label>
        <label><span>Trend vzorků [s]</span>
            <input type="number" id="trendsec">
        </label>
        <div class="buttons"><button onclick="savecfg()">Nastavit</button></div>
    </div>            
<hr>
    <label>
        <input type="checkbox" onchange="doScan.call(this)"> Nastavení WiFi
    </label>
    <div hidden="hidden" class="form">
            <label><span>SSID:</span>
                <input type="text"  id="ssid">            
            </label>
            <label><span>Heslo:</span>
                <input type="text" id="pass">            
            </label>
                <div class="buttons"><button onclick="savewifi()">Nastavit</button></div>
        <table class="scan">
            <thead>
                <tr><th>SSID</th><th>Signal</th></tr>
            </thead>
            <tbody id="scan_result">
                <tr>
                    <td colspan="2">(Vyhledávám sítě)</td>
                </tr>
            </tbody>
        </table>
    </div>            
</body>
</html>
)html");