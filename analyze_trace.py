import csv
import sys
import argparse
import os

PHASE_MAP = {
    0: "RequestArrival",
    1: "ServiceCompletion",
    2: "TimerExpiration",
    3: "ResponseArrival",
    4: "WorkloadGeneration"
}

CODE_MAP = {
    1: "BootstrapInjected",
    2: "WorkloadGenerated",
    3: "EventDiscarded",
    4: "RequestArrived",
    5: "RequestQueued",
    6: "ServiceStarted",
    7: "ServiceCompleted",
    8: "RequestCompleted",
    9: "RequestDropped"
}

def analyze_trace(file_path):
    print(f"Analyzing trace file: {file_path}")
    base_name = os.path.splitext(file_path)[0]
    txt_out = f"{base_name}_analysis.txt"
    html_out = f"{base_name}_timeline.html"

    # Track data for visualization
    request_states = {} 
    timeline_events = []

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            reader = list(csv.DictReader(f))
            
        with open(txt_out, 'w', encoding='utf-8') as out_f:
            out_f.write(f"Analyzing trace file: {file_path}\n")
            out_f.write("-" * 90 + "\n")
            out_f.write(f"{'Time':>6} | {'ReqID':>5} | {'Component':>9} | {'Phase':<20} | {'Action/Code':<25} | {'Field1 Details':<20}\n")
            out_f.write("-" * 90 + "\n")

            for row in reader:
                try:
                    time = int(row['time'])
                    req_id = int(row['request_id'])
                    phase_id = int(row['event_phase'])
                    comp_id = int(row['component_id'])
                    field0 = int(row['field0'])
                    field1 = int(row['field1'])
                except (ValueError, KeyError):
                    continue
                
                phase_name = PHASE_MAP.get(phase_id, f"Unknown({phase_id})")
                action_name = CODE_MAP.get(field0, f"CustomLog({field0})")
                
                details = f"Value={field1}"
                if field0 == 2:
                    details = f"WorkloadID={field1}"
                elif field0 == 3:
                    details = f"TokenID={field1}"
                elif field0 == 4:
                    details = f"Flag={field1}"
                elif field0 == 5:
                    details = f"QueueSize={field1}"
                elif field0 == 6 or field0 == 7:
                    details = f"SlotID={field1}"

                out_f.write(f"{time:6d} | {req_id:5d} | {comp_id:9d} | {phase_name:<20} | {action_name:<25} | {details:<20}\n")
                
                # Plot every request's active state until the next event
                if req_id in request_states:
                    prev = request_states[req_id]
                    if prev['start_time'] <= time:
                        timeline_events.append({
                            'comp': prev['comp_id'],
                            'req': req_id,
                            'phase': prev['phase_name'],
                            'action': prev['action_name'],
                            'details': prev['details'],
                            'start': prev['start_time'],
                            'end': time
                        })
                
                # terminal actions like completed/dropped
                if field0 in (3, 8, 9): 
                    timeline_events.append({
                        'comp': comp_id,
                        'req': req_id,
                        'phase': phase_name,
                        'action': action_name,
                        'details': details,
                        'start': time,
                        'end': time
                    })
                    request_states.pop(req_id, None)
                else: 
                    request_states[req_id] = {
                        'comp_id': comp_id,
                        'phase_name': phase_name,
                        'action_name': action_name,
                        'details': details,
                        'start_time': time
                    }

        print(f"-> Text analysis successfully written to {txt_out}")

        # Generate HTML Timeline
        generate_html_timeline(html_out, timeline_events)
        print(f"-> HTML Visualization successfully written to {html_out}")

    except FileNotFoundError:
        print(f"Error: File not found - {file_path}")

def generate_html_timeline(html_path, events):
    # Prepare raw events array for JavaScript
    events_js = "[\n"
    for ev in events:
        c = f"Component {ev['comp']}"
        r = f"Req {ev['req']}"
        start_ms = ev['start'] * 1000
        end_ms = ev['end'] * 1000
        if start_ms == end_ms:
            # make it minimally visible if instant
            end_ms += 0
        
        # Escape quotes
        d = str(ev['details']).replace("'", "\\'")
        p = str(ev['phase']).replace("'", "\\'")
        a = str(ev['action']).replace("'", "\\'")
        events_js += f"      {{ comp: '{c}', req: '{r}', start: {start_ms}, end: {end_ms}, phase: '{p}', action: '{a}', details: '{d}' }},\n"
    events_js += "    ]"

    html_content = f"""<!DOCTYPE html>
<html>
  <head>
    <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
    <script type="text/javascript">
      google.charts.load('current', {{'packages':['timeline']}});
      google.charts.setOnLoadCallback(drawChart);
      
      var rawEvents = {events_js};

      function drawChart() {{
        var container = document.getElementById('timeline');
        var chart = new google.visualization.Timeline(container);
        var dataTable = new google.visualization.DataTable();

        var groupBy = document.getElementById('groupBy').value;
        var reqFilter = document.getElementById('reqFilter').value.trim().toLowerCase();

        dataTable.addColumn({{ type: 'string', id: 'RowLabel' }});
        dataTable.addColumn({{ type: 'string', id: 'BarLabel' }});
        dataTable.addColumn({{ type: 'string', role: 'tooltip', p: {{ html: true }} }});
        dataTable.addColumn({{ type: 'date', id: 'Start' }});
        dataTable.addColumn({{ type: 'date', id: 'End' }});
        
        var rows = [];
        for (var i = 0; i < rawEvents.length; i++) {{
          var ev = rawEvents[i];
          
          if (reqFilter !== "") {{
             var reqNum = ev.req.replace('Req ', '');
             if (!ev.req.toLowerCase().includes(reqFilter) && reqNum !== reqFilter) {{
                 continue;
             }}
          }}

          var rowLabel = (groupBy === 'component') ? ev.comp : ev.req;
          var barLabel = ev.action; // Different color per Action/State
          
          var tooltip = '<div style="padding:12px; font-family:sans-serif; white-space:nowrap; border:1px solid #ccc; background:#fff; box-shadow: 0 2px 5px rgba(0,0,0,0.2);">' +
                        '<div style="font-size:16px;font-weight:bold;color:#333;margin-bottom:8px">' + ev.action + '</div>' +
                        '<table style="font-size:13px;color:#444">' +
                        '<tr><td style="padding-right:15px"><b>Request:</b></td><td>' + ev.req + '</td></tr>' +
                        '<tr><td><b>Component:</b></td><td>' + ev.comp + '</td></tr>' +
                        '<tr><td><b>Phase:</b></td><td>' + ev.phase + '</td></tr>' +
                        '<tr><td><b>Details:</b></td><td>' + ev.details + '</td></tr>' +
                        '</table>' +
                        '<div style="margin-top:8px;font-size:12px;color:#777;border-top:1px solid #eee;padding-top:4px">' + ev.start + 'ms - ' + ev.end + 'ms</div>' +
                        '</div>';

          rows.push([ 
            rowLabel, 
            barLabel, 
            tooltip,
            new Date(0,0,0,0,0,0, ev.start), 
            new Date(0,0,0,0,0,0, ev.end) 
          ]);
        }}
        
        if (rows.length === 0) {{
            container.innerHTML = "<p style='padding: 20px; color: red;'>No events match the current filter.</p>";
            return;
        }}

        dataTable.addRows(rows);

        var options = {{
          timeline: {{ showRowLabels: true }},
          hAxis: {{
            format: 'SSS ms',
          }},
          tooltip: {{ isHtml: true }}
        }};

        var uniqueRows = new Set(rows.map(r => r[0])).size;
        var dynamicHeight = Math.max(600, uniqueRows * 45 + 100);
        container.style.height = dynamicHeight + 'px';

        chart.draw(dataTable, options);
      }}
    </script>
    <style>
      body {{ font-family: sans-serif; margin: 20px; background: #fff; }}
      h1 {{ color: #333; margin-bottom: 5px; }}
      .controls {{ margin-bottom: 20px; padding: 15px; background: #f8f9fa; border: 1px solid #ddd; border-radius: 6px; box-shadow: 0 2px 4px rgba(0,0,0,0.05); }}
      .controls label {{ font-weight: 600; margin-right: 10px; color: #495057; }}
      .controls select, .controls input {{ padding: 8px 12px; margin-right: 25px; font-size: 14px; border: 1px solid #ced4da; border-radius: 4px; outline: none; transition: border-color .15s; }}
      .controls input:focus, .controls select:focus {{ border-color: #80bdff; }}
      #timeline {{ width: 100%; height: 600px; }}
      .header-desc {{ margin-bottom: 25px; color: #6c757d; font-size: 15px; }}
    </style>
  </head>
  <body>
    <h1>Simrun Detailed Network Trace</h1>
    <p class="header-desc">This chart visually maps every state transition for every request block. Hover over any bar to open the rich event inspector.</p>
    
    <div class="controls">
      <label for="groupBy">Group By:</label>
      <select id="groupBy" onchange="drawChart()">
        <option value="request">Request</option>
        <option value="component">Component</option>
      </select>
      
      <label for="reqFilter">Filter Request ID:</label>
      <input type="text" id="reqFilter" placeholder="e.g. 248" onkeyup="drawChart()">
    </div>

    <div id="timeline"></div>
  </body>
</html>
"""
    with open(html_path, 'w', encoding='utf-8') as f:
        f.write(html_content)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Analyze simrun design trace CSV to generate readable text and HTML visualization.")
    parser.add_argument("trace_path", type=str, nargs='?', default="build/complex_design_trace.csv", help="Path to the complex_design_trace.csv")
    args = parser.parse_args()
    analyze_trace(args.trace_path)
