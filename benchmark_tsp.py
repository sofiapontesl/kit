"""
benchmark_tsp.py
=================

Script de benchmark para o ILS+RVND do TSP (Capitulo 2 do kit-opt).

O QUE ELE FAZ:
  1. Roda o seu executavel (tsp.exe / ./tsp) varias vezes em cada instancia.
  2. Le a saida do programa (linha "Cost: <valor>") e mede o tempo de execucao.
  3. Compara o melhor custo e o custo medio encontrados com o valor OTIMO
     conhecido de cada instancia (tabela extraida do livro, secao 2.6).
  4. Calcula o GAP percentual: quanto sua solucao ficou acima do otimo.
  5. Salva tudo em um CSV e (se matplotlib estiver instalado) gera um
     grafico de barras com o gap de cada instancia.

COMO USAR (exemplo no Windows/PowerShell):
  python benchmark_tsp.py --exe .\tsp.exe --instances .\instancias --runs 5

COMO USAR (exemplo no Linux/Mac):
  python3 benchmark_tsp.py --exe ./tsp --instances ./instancias --runs 5

Por padrao, ele testa as instancias pequenas do capitulo 2 (as mesmas que
aparecem na Tabela 2.1 do livro). Voce pode escolher outras com --only.
"""

import argparse
import csv
import re
import subprocess
import sys
import time
from pathlib import Path
from statistics import mean, pstdev


# ---------------------------------------------------------------------------
# Tabela de valores OTIMOS conhecidos (TSPLIB / Tabela 2.1 do livro).
# Adicione mais instancias aqui se for testar outras.
# ---------------------------------------------------------------------------
KNOWN_OPTIMAL = {
    "burma14": 3323,
    "ch130": 6110,
    "ch150": 6528,
    "ulysses16": 6859,
    "ulysses22": 7013,
    "gr21": 2707,
    "gr24": 1272,
    "fri26": 937,
    "gr17": 2085,
    "bayg29": 1610,
    "bays29": 2020,
    "dantzig42": 699,
    "swiss42": 1273,
    "att48": 10628,
    "gr48": 5046,
    "hk48": 11461,
    "eil51": 426,
    "berlin52": 7542,
    "brazil58": 25395,
    "st70": 675,
    "eil76": 538,
    "pr76": 108159,
    "gr96": 55209,
    "gr120": 6942,
    "gr137": 69853,
    "rat99": 1211,
    "kroA100": 21282,
    "kroA150": 26524,
    "kroB100": 22141,
    "kroB150": 26130,
    "kroC100": 20749,
    "kroD100": 21294,
    "kroE100": 22068,
    "rd100": 7910,
    "eil101": 629,
    "lin105": 14379,
    "pr107": 44303,
    "pr124": 59030,
    "pr136": 96772,
    "pr144": 58537,
    "bier127": 118282,
    "pr152": 73682,
    "pr226": 80369,
    "a280": 2579,
}

# Instancias pequenas que rodam rapido, boas para um primeiro benchmark.
DEFAULT_INSTANCES = [
    "burma14", "dantzig42", "swiss42", "att48", "gr48",
    "eil51", "berlin52", "st70", "eil76", "rat99",
]

# Regex para extrair a linha "Cost: 1234" ou "Cost: 1234.5" da saida do programa.
COST_RE = re.compile(r"Cost:\s*([0-9]+(?:\.[0-9]+)?)")
# Regex para extrair a linha de rota, so para validar que e uma permutacao.
ROUTE_RE = re.compile(r"Route:\s*(.+)")


def find_instance_file(instances_dir: Path, name: str) -> Path | None:
    """Aceita tanto 'burma14' quanto 'burma14.tsp' como entrada."""
    candidate = instances_dir / name
    if candidate.exists():
        return candidate
    candidate = instances_dir / f"{name}.tsp"
    if candidate.exists():
        return candidate
    return None


def validate_route(route_str: str, n_expected: int) -> str | None:
    """
    Confere se a rota impressa e uma permutacao valida:
    - comeca e termina na mesma cidade
    - visita cada cidade de 1 a n exatamente uma vez (sem repetir, sem faltar)
    Retorna None se estiver tudo certo, ou uma mensagem de erro caso contrario.
    """
    parts = [p.strip() for p in route_str.split("-")]
    try:
        nodes = [int(p) for p in parts]
    except ValueError:
        return "rota contem valores nao numericos"

    if len(nodes) != n_expected + 1:
        return f"tamanho da rota incorreto: esperado {n_expected + 1}, veio {len(nodes)}"

    if nodes[0] != nodes[-1]:
        return "rota nao fecha (primeira cidade != ultima cidade)"

    visited = nodes[:-1]
    if sorted(visited) != list(range(1, n_expected + 1)):
        return "rota nao e uma permutacao valida de 1..n (cidade repetida ou faltando)"

    return None


def read_dimension(instance_path: Path) -> int | None:
    """Le o campo DIMENSION do cabecalho .tsp, para validar a rota depois."""
    try:
        text = instance_path.read_text(errors="ignore")
    except OSError:
        return None
    m = re.search(r"DIMENSION\s*:?\s*(\d+)", text)
    return int(m.group(1)) if m else None


def run_once(exe: Path, instance_path: Path, timeout: float) -> dict:
    """Roda o executavel uma vez e retorna metricas de uma execucao."""
    start = time.perf_counter()
    try:
        result = subprocess.run(
            [str(exe), str(instance_path)],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired:
        return {"ok": False, "error": f"timeout > {timeout}s", "elapsed": timeout}

    elapsed = time.perf_counter() - start
    output = result.stdout

    cost_match = COST_RE.search(output)
    if not cost_match:
        return {
            "ok": False,
            "error": "nao encontrei 'Cost:' na saida (verifique se o programa imprime isso)",
            "elapsed": elapsed,
            "raw_output": output[-500:],
        }

    cost = float(cost_match.group(1))

    route_match = ROUTE_RE.search(output)
    route_error = None
    if route_match:
        n = read_dimension(instance_path)
        if n:
            route_error = validate_route(route_match.group(1), n)

    return {
        "ok": True,
        "cost": cost,
        "elapsed": elapsed,
        "route_error": route_error,
    }


def benchmark_instance(exe: Path, instances_dir: Path, name: str, runs: int, timeout: float) -> dict:
    instance_path = find_instance_file(instances_dir, name)
    if instance_path is None:
        return {"name": name, "ok": False, "error": "arquivo de instancia nao encontrado"}

    costs, times, errors = [], [], []
    for run_idx in range(1, runs + 1):
        r = run_once(exe, instance_path, timeout)
        if not r["ok"]:
            errors.append(f"run {run_idx}: {r['error']}")
            continue
        costs.append(r["cost"])
        times.append(r["elapsed"])
        if r.get("route_error"):
            errors.append(f"run {run_idx}: ROTA INVALIDA -> {r['route_error']}")
        print(f"  [{name}] execucao {run_idx}/{runs}: custo={r['cost']:.2f}  tempo={r['elapsed']:.2f}s")

    if not costs:
        return {"name": name, "ok": False, "error": "; ".join(errors) or "nenhuma execucao valida"}

    optimal = KNOWN_OPTIMAL.get(name)
    best = min(costs)
    avg = mean(costs)
    std = pstdev(costs) if len(costs) > 1 else 0.0
    avg_time = mean(times)

    gap_best = ((best - optimal) / optimal * 100) if optimal else None
    gap_avg = ((avg - optimal) / optimal * 100) if optimal else None

    return {
        "name": name,
        "ok": True,
        "n_runs": len(costs),
        "optimal": optimal,
        "best_cost": best,
        "avg_cost": avg,
        "std_cost": std,
        "gap_best_%": gap_best,
        "gap_avg_%": gap_avg,
        "avg_time_s": avg_time,
        "errors": "; ".join(errors) if errors else "",
    }


def print_report(rows: list[dict]) -> None:
    header = f"{'instancia':<12} {'otimo':>8} {'melhor':>10} {'medio':>10} {'gap_melhor':>11} {'gap_medio':>10} {'tempo_med':>10} {'runs':>5}"
    print("\n" + "=" * len(header))
    print(header)
    print("-" * len(header))
    for r in rows:
        if not r["ok"]:
            print(f"{r['name']:<12}  FALHOU: {r['error']}")
            continue
        opt = f"{r['optimal']:.0f}" if r["optimal"] else "?"
        gap_b = f"{r['gap_best_%']:.2f}%" if r["gap_best_%"] is not None else "-"
        gap_a = f"{r['gap_avg_%']:.2f}%" if r["gap_avg_%"] is not None else "-"
        print(
            f"{r['name']:<12} {opt:>8} {r['best_cost']:>10.1f} {r['avg_cost']:>10.1f} "
            f"{gap_b:>11} {gap_a:>10} {r['avg_time_s']:>9.2f}s {r['n_runs']:>5}"
        )
        if r["errors"]:
            print(f"    aviso: {r['errors']}")
    print("=" * len(header))


def save_csv(rows: list[dict], path: Path) -> None:
    fieldnames = [
        "name", "ok", "n_runs", "optimal", "best_cost", "avg_cost", "std_cost",
        "gap_best_%", "gap_avg_%", "avg_time_s", "errors", "error",
    ]
    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        for r in rows:
            writer.writerow(r)
    print(f"\nCSV salvo em: {path}")


def save_chart(rows: list[dict], path: Path) -> None:
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("(matplotlib nao instalado - pulei a geracao do grafico. "
              "Rode 'pip install matplotlib' se quiser o grafico.)")
        return

    valid = [r for r in rows if r["ok"] and r["gap_avg_%"] is not None]
    if not valid:
        print("(sem dados suficientes para gerar grafico de gap)")
        return

    names = [r["name"] for r in valid]
    gaps_best = [r["gap_best_%"] for r in valid]
    gaps_avg = [r["gap_avg_%"] for r in valid]

    x = range(len(names))
    width = 0.35
    fig, ax = plt.subplots(figsize=(max(6, len(names) * 0.9), 5))
    ax.bar([i - width / 2 for i in x], gaps_best, width, label="Gap (melhor execucao)")
    ax.bar([i + width / 2 for i in x], gaps_avg, width, label="Gap (media das execucoes)")
    ax.axhline(0, color="black", linewidth=0.8)
    ax.set_ylabel("Gap em relacao ao otimo (%)")
    ax.set_title("Gap do ILS+RVND em relacao ao otimo conhecido, por instancia")
    ax.set_xticks(list(x))
    ax.set_xticklabels(names, rotation=45, ha="right")
    ax.legend()
    fig.tight_layout()
    fig.savefig(path, dpi=150)
    print(f"Grafico salvo em: {path}")


def main():
    parser = argparse.ArgumentParser(description="Benchmark do ILS+RVND para o TSP")
    parser.add_argument("--exe", required=True, type=Path,
                         help="Caminho do executavel (ex: .\\tsp.exe ou ./tsp)")
    parser.add_argument("--instances", required=True, type=Path,
                         help="Pasta com os arquivos .tsp (ex: .\\instancias)")
    parser.add_argument("--runs", type=int, default=5,
                         help="Quantas vezes rodar cada instancia (padrao: 5)")
    parser.add_argument("--timeout", type=float, default=120.0,
                         help="Timeout em segundos por execucao (padrao: 120)")
    parser.add_argument("--only", nargs="*", default=None,
                         help="Lista de instancias especificas (ex: --only burma14 att48). "
                              "Se omitido, usa a lista padrao de instancias pequenas.")
    parser.add_argument("--out", type=Path, default=Path("benchmark_resultado.csv"),
                         help="Arquivo CSV de saida")
    parser.add_argument("--chart", type=Path, default=Path("benchmark_gap.png"),
                         help="Arquivo PNG do grafico de gap")
    args = parser.parse_args()

    if not args.exe.exists():
        print(f"ERRO: executavel nao encontrado em {args.exe}")
        sys.exit(1)
    if not args.instances.exists():
        print(f"ERRO: pasta de instancias nao encontrada em {args.instances}")
        sys.exit(1)

    instances_to_test = args.only if args.only else [f.stem for f in args.instances.glob("*.tsp")]

    print(f"Executavel: {args.exe}")
    print(f"Instancias: {args.instances}")
    print(f"Execucoes por instancia: {args.runs}")
    print(f"Testando: {', '.join(instances_to_test)}\n")

    rows = []
    for name in instances_to_test:
        print(f"-> Instancia: {name}")
        row = benchmark_instance(args.exe, args.instances, name, args.runs, args.timeout)
        rows.append(row)

    print_report(rows)
    save_csv(rows, args.out)
    save_chart(rows, args.chart)


if __name__ == "__main__":
    main()