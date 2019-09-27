import csv
import json

with open('invTypes.csv', newline='') as csvfile:
    dialect = csv.Sniffer().sniff(csvfile.read(10000))
    csvfile.seek(0)
    reader = csv.reader(csvfile, dialect)
    firstrow = next(reader)
    l = list()
    for row in reader:
        entry = dict()
        for i, item in enumerate(row):
            key = firstrow[i]
            if i == 0:
                item = int(item)
            entry[key] = item
        l.append(entry)

with open("invTypes.json", 'w') as jsonfile:
    jsonfile.write(json.dumps(l))

