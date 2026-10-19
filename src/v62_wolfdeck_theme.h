#pragma once
#include <Arduino.h>
static const char WOLFDECK_THEME[] PROGMEM=R"CSS(
:root{color-scheme:dark;scrollbar-color:#60dfff #081a2b;scrollbar-width:auto}html{overflow-y:scroll;scrollbar-gutter:stable}*::-webkit-scrollbar{width:14px}*::-webkit-scrollbar-track{background:#081a2b}*::-webkit-scrollbar-thumb{background:#389db8;border:3px solid #081a2b;border-radius:9px}#out{max-height:65vh;overflow-y:auto;scrollbar-gutter:stable}html{min-height:100%;background:#030912}
body{min-height:100vh;box-sizing:border-box;color:#edf8ff!important;font-family:system-ui,sans-serif!important;background-color:#030912!important;background-image:linear-gradient(100deg,rgba(2,7,16,.82),rgba(2,7,16,.62)),url('/wolfdeck_web_background.jpg')!important;background-position:center!important;background-size:cover!important;background-attachment:fixed!important;background-repeat:no-repeat!important}
h1{color:#7ce5ff!important;letter-spacing:.08em;text-shadow:0 2px 10px #000}
h2,h3,label,p{color:#edf8ff}a{color:#8fe7ff!important}
section,.card,.panel,.tile,nav{background:rgba(3,14,30,.92)!important;border:1px solid #246581!important;border-radius:12px;padding:14px;box-shadow:0 8px 28px #0006}
input,button,textarea,select{background:#0b2138!important;color:#fff!important;border:1px solid #448ca9!important;max-width:100%}
input::placeholder,textarea::placeholder{color:#b5cbdc;opacity:1}
button{font-weight:650;min-height:44px}button[aria-pressed="true"]{background:#155069!important;border-color:#8eeaff!important}button:hover{background:#15445d!important}
:focus-visible{outline:3px solid #69deff;outline-offset:3px}
nav{display:flex;gap:8px;flex-wrap:wrap}nav a{padding:8px;text-decoration:none;border:1px solid #2d6683;border-radius:6px}
pre{background:#03111f;color:#e5f5ff;padding:12px;border-radius:8px;white-space:pre-wrap;overflow-wrap:anywhere}
select option{background:#0b2138;color:#fff}#out:empty{display:none}
@media(max-width:600px){body{padding:12px!important;background-attachment:scroll!important}h1{font-size:1.9rem}nav{padding:10px}}
)CSS";
