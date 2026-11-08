#!/usr/bin/env python3
"""Convert ESPHome SGTRACE log lines to a standalone position graph and CSV."""
import argparse
import csv
import html
import json
import re
from pathlib import Path

FIELDS = (
    'ms', 'position', 'direction', 'speed', 'result', 'software_sgthrs',
    'driver_sgthrs', 'tcoolthrs', 'obstacle_armed', 'position_known',
    'target_pos', 'max_speed', 'accel', 'phase', 'consec_stalls',
    'steps_since_arm', 'diag_pin', 'obstacle_enabled', 'min_speed', 'stop_code',
)
FLOAT_INDICES = {3, 11, 12, 18}
PATTERN = re.compile(
    r'SGTRACE,(\d+),(-?\d+),(-?\d+),([\d.-]+),(-?\d+),(\d+),(-?\d+),(-?\d+),([01]),([01])'
    r'(?:,(-?\d+),([\d.-]+),([\d.-]+),(\d+),(\d+),(-?\d+),([01]),([01]),([\d.-]+),(\d+))?'
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path, help='saved ESPHome log')
    parser.add_argument('--output', type=Path, default=Path('stallguard-trace.html'))
    args = parser.parse_args()
    rows = []
    for line in args.log.read_text(encoding='utf-8', errors='replace').splitlines():
        match = PATTERN.search(line)
        if match:
            groups = match.groups()
            values = [
                (float(v) if i in FLOAT_INDICES else int(v)) if v is not None else 0
                for i, v in enumerate(groups)
            ]
            rows.append(dict(zip(FIELDS, values)))
    if not rows:
        parser.error('No SGTRACE samples found. Enable StallGuard Recording and save the ESPHome log during movement.')
    csv_path = args.output.with_suffix('.csv')
    with csv_path.open('w', newline='', encoding='utf-8') as out:
        writer = csv.DictWriter(out, fieldnames=FIELDS)
        writer.writeheader()
        writer.writerows(rows)
    data = json.dumps(rows).replace('<', '\\u003c')
    page = '''<!doctype html><meta charset="utf-8"><title>StallGuard travel trace</title>
<style>body{font:15px system-ui;max-width:1150px;margin:2em auto;padding:0 1em;color:#182338}svg{width:100%;background:#f7f9fc;border:1px solid #ccd4df}label{margin-right:1em}p{line-height:1.5}.key{font-size:13px}</style>
<h1>StallGuard along travel</h1><p>Each dot is a real UART reading. Position is the commanded step counter, not a measured door position. Low readings mean higher load. The dashed line is twice the software sensitivity setting; protection is only active under its own conditions.</p>
<label>Direction <select id="dir"><option value="all">Both</option><option value="1">Forward</option><option value="-1">Backward</option></select></label><label><input id="known" type="checkbox"> Only known positions</label>
<p class="key">Blue: forward · orange: backward · red dashed: software trigger level · hover a dot for details</p><svg id="chart" viewBox="0 0 1000 500" role="img" aria-label="StallGuard result versus position"></svg><p id="info"></p><p>SG_RESULT depends on speed, current, and acceleration. Sampling can miss brief dips and the event that stopped the motor. The driver threshold and TCOOLTHRS in the CSV are firmware shadow values, not hardware readback. SGTRACE also records whether ordinary-move obstacle detection was armed; this does not describe homing or guarantee detection.</p>
<script>const data=DATA;const svg=document.querySelector('#chart');const NS='http://www.w3.org/2000/svg';function add(tag,attrs,text){let e=document.createElementNS(NS,tag);for(let [k,v] of Object.entries(attrs))e.setAttribute(k,v);if(text!==undefined)e.textContent=text;svg.appendChild(e);return e}
function draw(){svg.replaceChildren();let d=data.filter(p=>(dir.value==='all'||p.direction==+dir.value)&&(!known.checked||p.position_known));info.textContent=d.length+' samples shown of '+data.length+'.';if(!d.length)return;let xmin=Math.min(...d.map(p=>p.position)),xmax=Math.max(...d.map(p=>p.position));if(xmin===xmax)xmax++;let ymax=Math.max(100,...d.map(p=>Math.max(p.result,p.software_sgthrs*2)));let x=v=>70+(v-xmin)/(xmax-xmin)*900,y=v=>440-v/ymax*400;for(let i=0;i<=5;i++){let v=ymax*i/5;add('line',{x1:70,y1:y(v),x2:970,y2:y(v),stroke:'#d9e0e8'});add('text',{x:60,y:y(v)+5,'text-anchor':'end','font-size':12},v.toFixed(0))}add('text',{x:500,y:490,'text-anchor':'middle'},'Commanded position (steps)');add('text',{x:15,y:230,transform:'rotate(-90 15 230)','text-anchor':'middle'},'SG_RESULT / trigger level');add('text',{x:70,y:462,'font-size':12},xmin);add('text',{x:970,y:462,'text-anchor':'end','font-size':12},xmax);for(let p of d){let yy=y(p.software_sgthrs*2);add('line',{x1:x(p.position)-3,y1:yy,x2:x(p.position)+3,y2:yy,stroke:'#c83232','stroke-dasharray':'3 2'});let dot=add('circle',{cx:x(p.position),cy:y(p.result),r:3,fill:p.direction>0?'#1764c0':'#dc791e',opacity:.8});let title=document.createElementNS(NS,'title');title.textContent=Object.entries(p).map(([k,v])=>k+': '+v).join('\\n');dot.appendChild(title)}}dir.onchange=draw;known.onchange=draw;draw();</script>'''.replace('DATA', data)
    args.output.write_text(page, encoding='utf-8')
    print(f'{len(rows)} samples: {args.output} and {csv_path}')


if __name__ == '__main__':
    main()
