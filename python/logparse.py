#!/usr/bin/env python3

import sys, math
from collections import Counter

LOG_SEVERITY_LEVELS = ["INFO", "WARN", "ERROR", "FATAL"]

class Record:
    def __init__(self, line):
        line = line.strip()
        items = line.split()

        # Initialize fixed fields
        self.time = items[0]
        self.level = items[1]
        self.component = items[2]
        self.event = items[3]
        # Initialize dictionary
        self.kv_pairs = {}
        for i in range(4, len(items)):
            key, value = items[i].split('=', 1)
            self.kv_pairs[key] = value

        self.isValid = self.level in LOG_SEVERITY_LEVELS

class LogFile:
    def __init__(self, text):
        self.level_counts = {level: 0 for level in LOG_SEVERITY_LEVELS}
        self.event_counts = Counter()
        self.errors = Counter()
        self.request_durations = []
        self.records = []
        
        line_num = 0
        for line in [x.strip() for x in text.splitlines()]:
            try:
                self.records.append(Record(line))
            except Exception as e:
                print(f"Error on line {line_num:3}:  {line}")
            line_num += 1

    def process(self):
        for rec in self.records:
            if not rec.isValid:
                continue
            
            self.level_counts[rec.level] += 1

            event = '.'.join([rec.component, rec.event])
            self.event_counts[event] += 1

            if rec.level == "ERROR":
                self.errors[event] += 1

            if rec.event == "req_done" and 'dur_ms' in rec.kv_pairs:
                self.request_durations.append(int(rec.kv_pairs['dur_ms']))


    def print_items(self, title, items):
        print(title, end=' ')
        for item in items:
            print(item, end=' ')
        print()

    def print_summary(self):
        severities = ['='.join([level, str(self.level_counts[level])]) for level in self.level_counts]
        top_events_list = sorted(self.event_counts.items(), key=lambda x: x[1], reverse=True)[:3]
        top_events_strs = ['='.join([ev[0], str(ev[1])]) for ev in top_events_list]
        errors = ['='.join([err, str(self.errors[err])]) for err in self.errors]

        self.request_durations.sort()
        index = math.ceil(0.95 * len(self.request_durations)) - 1
        p95_duration = self.request_durations[index]
        avg_duration = sum(self.request_durations) / len(self.request_durations)

        self.print_items("Severity levels:   ", severities)
        self.print_items("Top events:        ", top_events_strs)
        self.print_items("Errors:            ", errors)
        print(          f"Request avg / P95:  {avg_duration:.2f} ms / {p95_duration:.2f} ms")

def main():
    text = None
    # if len(sys.argv) == 2:
    #     with open(sys.argv[1]) as f:
    #         text = f.read()
    # else:
    #     text = sys.stdin.read()

    text =  """2026-02-08T18:41:03.120Z INFO  auth   login_ok    user=alice ip=10.0.0.4 dur_ms=12 
            2026-02-08T18:41:05.003Z WARN  auth   login_fail  user=alice ip=10.0.0.4 reason=bad_pw 
            2026-02-08T18:41:05.410Z WARN  auth   login_fail  user=alice ip=10.0.0.4 reason=bad_pw 
            2026-02-08T18:41:06.001Z INFO  api    req_done   method=GET path=/v1/items status=200 dur_ms=34 
            2026-02-08T18:41:06.090Z INFO  api    req_done   method=GET path=/v1/items status=200 dur_ms=29 
            2026-02-08T18:41:06.200Z INFO  api    req_done   method=POST path=/v1/items status=500 dur_ms=91 err=DBTimeout 
            2026-02-08T18:41:06.201Z ERROR api    req_done   method=POST path=/v1/items status=500 dur_ms=91 err=DBTimeout 
            2026-02-08T18:41:07.000Z INFO  db     query      sql=select_users dur_ms=44 
            2026-02-08T18:41:07.100Z WARNdb     querysql=select_userdur_ms=20slow=true
            sdflkj123   sdlfkjlkj``32`3`

            k
            2026-02-08T18:41:07.100Z WARN  db     query      sql=select_users dur_ms=208 slow=true 
            2026-02-08T18:41:07.500Z INFO  api    req_done   method=GET path=/health status=200 dur_ms=2 
            2026-02-08T18:41:08.010Z ERROR auth   token_fail user=bob ip=10.0.0.7 err=ExpiredToken 
            2026-02-08T18:41:08.011Z ERROR auth   token_fail user=bob ip=10.0.0.7 err=ExpiredToken 
            2026-02-08T18:41:09.000Z INFO  api    req_done   method=GET path=/v1/items status=200 dur_ms=31"""

    logfile = LogFile(text)
    logfile.process()
    logfile.print_summary()


if __name__ == "__main__":
    main()
