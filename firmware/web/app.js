fetch('/api/status').then(r => r.json()).then(s => {
  document.getElementById('model').textContent = `${s.model} ${s.rated_kva} kVA - ${s.status}`;
  const rows = [['Output V', s.output_voltage.join(' / ')], ['Output A', s.output_current.join(' / ')],
                ['Load', s.load_pct + ' %'], ['Battery', s.battery_pct + ' %'], ['Runtime', s.runtime_min + ' min']];
  document.getElementById('t').innerHTML = rows.map(([k, v]) => `<tr><td>${k}</td><td>${v}</td></tr>`).join('');
});
