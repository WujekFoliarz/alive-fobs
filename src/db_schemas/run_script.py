import subprocess
from pathlib import Path
import sys

schema_dir = Path.cwd()
DDL2CPP = str(schema_dir) +  "/sqlpp23-ddl2cpp"

for sql_file in schema_dir.rglob("*.sql"):
    out_file = sql_file.with_suffix(".hpp")

    cmd = [
        DDL2CPP,
        "--path-to-ddl", str(sql_file),
        "--path-to-header", str(out_file),
        "--namespace", "table",
        "--generate-table-creation-helper"
    ]

    print("Running:", " ".join(cmd))
    subprocess.run(cmd, check=True)
