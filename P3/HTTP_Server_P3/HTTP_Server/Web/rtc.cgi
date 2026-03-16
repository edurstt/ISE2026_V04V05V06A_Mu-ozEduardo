t <html>
t <head>
t <title>RTC - ISE ETSIST</title>
t <meta http-equiv="refresh" content="1">
t <style>
t   .rtc-card {
t     max-width: 500px;
t     margin: 40px auto;
t     background: #ffffff;
t     border-radius: 12px;
t     box-shadow: 0 8px 20px rgba(0,0,0,0.15);
t     overflow: hidden;
t     border: 1px solid #003d7c;
t   }
t   .rtc-header {
t     background: #003d7c;
t     color: white;
t     padding: 15px;
t     text-align: center;
t     font-weight: bold;
t     font-size: 1.2em;
t   }
t   .rtc-body { padding: 20px; }
t   .rtc-row {
t     display: flex;
t     justify-content: space-between;
t     align-items: center;
t     padding: 15px 10px;
t     border-bottom: 1px solid #eee;
t   }
t   .rtc-row:last-child { border-bottom: none; }
t   .label { color: #555; font-weight: 600; text-transform: uppercase; font-size: 0.85em; }
t   .value { color: #003d7c; font-size: 1.6em; font-family: 'Courier New', monospace; font-weight: bold; }
t   .date-value { font-size: 1.3em; color: #444; }
t   .sync-msg { text-align: center; font-size: 0.75em; color: #888; padding: 10px; }
t </style>
t </head>
i pg_header.inc
t <div class="rtc-card">
t   <div class="rtc-header">ESTADO DEL RELOJ (RTC)</div>
t   <div class="rtc-body">
t     <div class="rtc-row">
t       <span class="label">Hora Actual</span>
c h 1 <span class="value">%s</span>
t     </div>
t     <div class="rtc-row">
t       <span class="label">Fecha Sistema</span>
c h 2 <span class="date-value">%s</span>
t     </div>
t   </div>
t   <div class="sync-msg">Sincronizado v&iacute;a SNTP cada 3 minutos</div>
t </div>
i pg_footer.inc
.