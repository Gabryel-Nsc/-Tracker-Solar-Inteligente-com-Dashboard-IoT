#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>

// ==========================================
// VARIÁVEIS EXTERNAS
// ==========================================
extern WebServer server;
extern int valCE, valCD, valBE, valBD;
extern int posicaoAzimutal, posicaoVertical;
extern bool modoManualAtivo, modoUmEixo;

extern int toleranciaLuz, toleranciaEfetiva;
extern bool usarToleranciaDinamica;
extern float fatorK, alfaFiltro;
extern long intervaloMovimento;

extern int offsetCE, offsetCD, offsetBE, offsetBD;
extern float Kp, Ki, Kd;

// ==========================================
// CREDENCIAIS DE AUTENTICAÇÃO
// ==========================================
const char* auth_user = "admin";
const char* auth_pass = "1234";

inline bool cookies_autenticado = false;

// ==========================================
// INTERFACES HTML (Armazenadas na Flash)
// ==========================================
const char LOGIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Login - Solar Tracker</title>
  <style>
    body { font-family: 'Segoe UI', sans-serif; background: #0f172a; color: #fff; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
    .box { background: #1e293b; padding: 40px; border-radius: 12px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); text-align: center; width: 320px; border: 1px solid #334155; }
    input { width: 90%; padding: 12px; margin: 15px 0; border: 1px solid #475569; border-radius: 8px; background: #0f172a; color: #fff; font-size: 16px; outline: none; }
    input:focus { border-color: #f59e0b; }
    button { width: 100%; padding: 12px; background: #f59e0b; color: #1e293b; border: none; border-radius: 8px; font-weight: bold; font-size: 16px; cursor: pointer; transition: 0.3s; }
    button:hover { background: #d97706; }
    h2 { margin-top: 0; color: #e2e8f0; font-size: 24px; letter-spacing: 1px; }
  </style>
</head>
<body>
  <div class="box">
    <h2>SOLAR TRACKER</h2>
    <form method="POST" action="/login">
      <input type="text" name="username" placeholder="Usuário" required><br>
      <input type="password" name="password" placeholder="Senha" required><br>
      <button type="submit">Acessar Sistema</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

const char RASTREADOR_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Solar Tracker</title>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    :root { --bg: #0f172a; --card: #1e293b; --primary: #f59e0b; --text: #f8fafc; --text-muted: #94a3b8; }
    body { font-family: 'Segoe UI', Tahoma, sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 20px; text-align: center; }
    .container { max-width: 1000px; margin: 0 auto; }
    
    h1 { margin-bottom: 5px; font-size: 2.2rem; color: #e2e8f0; letter-spacing: 1.5px; text-transform: uppercase; }
    h2 { font-size: 1.2rem; color: var(--primary); font-weight: normal; margin-top: 0; margin-bottom: 20px; letter-spacing: 2px; text-transform: uppercase; }
    
    .card { background: var(--card); padding: 20px; border-radius: 12px; margin-bottom: 20px; box-shadow: 0 4px 15px rgba(0,0,0,0.3); border: 1px solid #334155;}
    
    /* Layout Superior dividido: Telemetria (Esquerda) e 3D (Direita) */
    .top-layout { display: grid; grid-template-columns: 350px 1fr; gap: 20px; margin-bottom: 20px; align-items: stretch; }
    @media(max-width: 850px) { .top-layout { grid-template-columns: 1fr; } }
    
    #canvas3d { width: 100%; height: 40vh; min-height: 250px; background: #0f172a; border-radius: 10px; overflow: hidden; margin-bottom: 10px; box-shadow: inset 0 0 20px rgba(0,0,0,0.5); border: 1px solid #334155;}
    
    .tabs { display: flex; justify-content: center; gap: 10px; margin-bottom: 20px; flex-wrap: wrap; }
    .tab-btn { background: var(--card); color: var(--text-muted); border: 2px solid transparent; padding: 12px 24px; border-radius: 8px; font-size: 15px; font-weight: 600; cursor: pointer; transition: 0.3s; }
    .tab-btn:hover { color: var(--text); background: #27354c; }
    .tab-btn.active { background: #27354c; color: var(--primary); border-bottom: 2px solid var(--primary); }
    
    .tab-content { display: none; animation: fadeIn 0.4s; }
    .tab-content.active { display: block; }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(5px); } to { opacity: 1; transform: translateY(0); } }

    .btn-group { display: flex; justify-content: center; gap: 15px; margin-top: 15px; }
    .btn { padding: 12px 25px; border-radius: 8px; border: none; font-weight: bold; font-size: 15px; cursor: pointer; transition: 0.3s; color: white; }
    .btn-on { background: #16a34a; } .btn-on:hover { background: #15803d; }
    .btn-off { background: #dc2626; } .btn-off:hover { background: #b91c1c; }
    .btn-cal { background: #0284c7; } .btn-cal:hover { background: #0369a1; }
    
    .slider-container { text-align: left; margin-bottom: 20px; }
    .slider-container label { font-size: 1.05rem; font-weight: 600; color: #cbd5e1; }
    .slider-val { float: right; color: var(--primary); font-weight: bold; font-size: 1.1rem; }
    input[type=range] { width: 100%; margin-top: 10px; accent-color: var(--primary); cursor: pointer; }
    
    .toggle-box { display: flex; justify-content: space-between; align-items: center; background: #0f172a; padding: 12px 18px; border-radius: 8px; border: 1px solid #334155; margin-bottom: 20px; }
    .toggle-box span { font-weight: 600; color: #cbd5e1; }

    .pid-container { display: flex; justify-content: space-between; gap: 10px; margin-bottom: 20px; background: #0f172a; padding: 15px; border-radius: 8px; border: 1px solid #334155; }
    .pid-box { flex: 1; text-align: center; }
    .pid-box label { display: block; font-weight: bold; color: #cbd5e1; margin-bottom: 5px; }
    .pid-box input { width: 80%; padding: 8px; border-radius: 6px; border: 1px solid #475569; background: #1e293b; color: white; text-align: center; font-size: 1rem; outline: none;}

    /* Grades e Cards Otimizados - Voltam para 2 colunas */
    .plotter-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 15px; }
    @media(max-width: 600px) { .plotter-grid { grid-template-columns: 1fr; } }
    
    .plot-card { background: #0f172a; padding: 12px; border-radius: 10px; border: 1px solid #334155; text-align: left; }
    .plot-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; font-size: 0.9rem; font-weight: bold; color: var(--text-muted); text-transform: uppercase; }
    .plot-val { color: var(--primary); font-size: 1.2rem; }
    /* Aumento da altura dos gráficos para torná-los maiores e mais visíveis */
    .canvas-container { position: relative; height: 130px; width: 100%; }

    /* Estilos do painel de telemetria lateral */
    .telemetry-grid { display: grid; grid-template-columns: 1fr; gap: 8px; }
    .telemetry-box { background: #0f172a; padding: 12px; border-radius: 8px; border: 1px solid #334155; display: flex; justify-content: space-between; align-items: center; }
    .telemetry-label { color: #cbd5e1; font-weight: bold; font-size: 0.9rem; }
    .telemetry-val { color: white; font-size: 1.1rem; font-weight: bold; }
    .badge { padding: 4px 10px; border-radius: 12px; font-size: 0.8rem; font-weight: bold; color: white; }
    .bg-ok { background: #16a34a; } .bg-warn { background: #dc2626; }
    
    .footer-link { color: var(--text-muted); text-decoration: none; font-size: 14px; margin-top: 10px; display: inline-block; }
  </style>
</head>
<body>
  <div class="container">
    <h1>SOLAR TRACKER</h1>
    <h2>SIMULAÇÃO 3D</h2>

    <div class="top-layout">
      <!-- Painel Esquerdo: Telemetria e Status -->
      <div class="card" style="margin-bottom: 0; display: flex; flex-direction: column; justify-content: center;">
        <h3 style="color: var(--primary); margin-top:0; font-size:1.1rem; text-align:left;">Diagnóstico de Malha</h3>
        
        <div class="telemetry-grid" style="margin-bottom: 15px;">
          <div class="telemetry-box">
            <span class="telemetry-label">Média Cima:</span> <span class="telemetry-val" id="valMedC">--</span>
          </div>
          <div class="telemetry-box">
            <span class="telemetry-label">Média Baixo:</span> <span class="telemetry-val" id="valMedB">--</span>
          </div>
          <div class="telemetry-box">
            <span class="telemetry-label">Média Esquerda:</span> <span class="telemetry-val" id="valMedE">--</span>
          </div>
          <div class="telemetry-box">
            <span class="telemetry-label">Média Direita:</span> <span class="telemetry-val" id="valMedD">--</span>
          </div>
        </div>

        <div class="telemetry-grid">
          <div class="telemetry-box" style="background:#1e293b; border-color:#475569;">
            <div style="text-align:left;">
                <div class="telemetry-label" style="color:var(--primary)">ERRO VERTICAL</div>
                <div style="font-size:0.75rem; color:#94a3b8">Cima/Baixo</div>
            </div>
            <div style="text-align:right;">
                <span class="telemetry-val" id="valErrV" style="margin-right:8px; font-size:1rem;">--</span>
                <span id="badgeV" class="badge bg-ok">ALINHADO</span>
            </div>
          </div>
          <div class="telemetry-box" style="background:#1e293b; border-color:#475569;">
            <div style="text-align:left;">
                <div class="telemetry-label" style="color:var(--primary)">ERRO HORIZONTAL</div>
                <div style="font-size:0.75rem; color:#94a3b8">Esq/Dir</div>
            </div>
            <div style="text-align:right;">
                <span class="telemetry-val" id="valErrH" style="margin-right:8px; font-size:1rem;">--</span>
                <span id="badgeH" class="badge bg-ok">ALINHADO</span>
            </div>
          </div>
        </div>
      </div>

      <!-- Painel Direito: Simulação 3D -->
      <div class="card" style="padding: 15px; margin-bottom: 0;">
        <div id="canvas3d"></div>
        <div style="font-size: 1.1rem; margin-top: 10px;">Modo Atual: <span id="modoTxt" style="color:var(--primary); font-weight:bold;">--</span></div>
      </div>
    </div>

    <!-- Navegação em Abas -->
    <div class="tabs">
      <button class="tab-btn active" onclick="openTab('tab-acoes', this)">Modo de Operação</button>
      <button class="tab-btn" onclick="openTab('tab-manual', this)">Controle Manual</button>
      <button class="tab-btn" onclick="openTab('tab-sensores', this)">Telemetria Avançada</button>
      <button class="tab-btn" onclick="openTab('tab-config', this)">Filtros e Calibração</button>
    </div>

    <div id="tab-acoes" class="tab-content active">
      <div class="card">
        <p style="color: var(--text-muted);">Alterne o modo de funcionamento do sistema principal.</p>
        <div class="btn-group">
          <button id="btnAuto" class="btn btn-on" onclick="setModo(0)">ATIVAR AUTOMÁTICO</button>
          <button id="btnMan" class="btn btn-off" onclick="setModo(1)">ATIVAR MANUAL</button>
        </div>
      </div>
      
      <div class="card">
        <p style="color: var(--text-muted);">Configuração de Rastreamento (Eixos).</p>
        <div class="btn-group">
          <button id="btn1Eixo" class="btn btn-off" onclick="setEixos(1)">1 EIXO (AZIMUTAL)</button>
          <button id="btn2Eixos" class="btn btn-on" onclick="setEixos(0)">2 EIXOS (COMPLETO)</button>
        </div>
      </div>
    </div>

    <div id="tab-manual" class="tab-content">
      <div class="card" style="max-width: 600px; margin: 0 auto;">
        <p style="color: var(--text-muted); margin-bottom: 20px;">Use os controles abaixo para mover a maquete e o hardware real simultaneamente.</p>
        
        <div class="slider-container">
          <label>Base Rotativa (Azimute)</label>
          <span class="slider-val"><span id="lblH">90</span>°</span>
          <input type="range" id="rngH" min="15" max="165" value="90" oninput="updateSliders()">
        </div>
        
        <div class="slider-container">
          <label>Inclinação (Elevação)</label>
          <span class="slider-val"><span id="lblV">45</span>°</span>
          <input type="range" id="rngV" min="0" max="90" value="45" oninput="updateSliders()">
        </div>
      </div>
    </div>

    <!-- TELA DE TELEMETRIA EXPANDIDA (Gráficos maiores e agrupados) -->
    <div id="tab-sensores" class="tab-content">
      <div class="card">
        <h3 style="color: var(--primary); margin-top:0; font-size:1.1rem; text-align:left;">Sensores Individuais (LDR)</h3>
        <div class="plotter-grid">
          <div class="plot-card">
            <div class="plot-header"><span>Cima Esq.</span> <span class="plot-val" id="txtCE">--</span></div>
            <div class="canvas-container"><canvas id="chartCE"></canvas></div>
          </div>
          <div class="plot-card">
            <div class="plot-header"><span>Cima Dir.</span> <span class="plot-val" id="txtCD">--</span></div>
            <div class="canvas-container"><canvas id="chartCD"></canvas></div>
          </div>
          <div class="plot-card">
            <div class="plot-header"><span>Baixo Esq.</span> <span class="plot-val" id="txtBE">--</span></div>
            <div class="canvas-container"><canvas id="chartBE"></canvas></div>
          </div>
          <div class="plot-card">
            <div class="plot-header"><span>Baixo Dir.</span> <span class="plot-val" id="txtBD">--</span></div>
            <div class="canvas-container"><canvas id="chartBD"></canvas></div>
          </div>
        </div>

        <h3 style="color: var(--primary); font-size:1.1rem; text-align:left; border-top:1px solid #334155; padding-top:15px; margin-top: 15px;">Atuação Física (Servomotores)</h3>
        <div class="plotter-grid">
          <div class="plot-card">
            <div class="plot-header"><span>Azimute (Horizontal)</span> <span class="plot-val"><span id="txtPosH">--</span>°</span></div>
            <div class="canvas-container"><canvas id="chartPosH"></canvas></div>
          </div>
          <div class="plot-card">
            <div class="plot-header"><span>Elevação (Vertical)</span> <span class="plot-val"><span id="txtPosV">--</span>°</span></div>
            <div class="canvas-container"><canvas id="chartPosV"></canvas></div>
          </div>
        </div>
      </div>
    </div>

    <div id="tab-config" class="tab-content">
      <div class="card" style="max-width: 600px; margin: 0 auto;">
        
        <h3 style="color: #e2e8f0; margin-top: 0; text-align: left;">Controlador PID</h3>
        <p style="color: var(--text-muted); font-size: 0.9rem; text-align: left; margin-top: 0;">Sintonia fina dos ganhos de correção do motor.</p>
        
        <div class="pid-container">
          <div class="pid-box">
            <label>Kp (Proporcional)</label>
            <input type="number" id="valKp" step="0.001" onchange="updatePID()">
          </div>
          <div class="pid-box">
            <label>Ki (Integral)</label>
            <input type="number" id="valKi" step="0.001" onchange="updatePID()">
          </div>
          <div class="pid-box">
            <label>Kd (Derivativo)</label>
            <input type="number" id="valKd" step="0.001" onchange="updatePID()">
          </div>
        </div>

        <hr style="border: 0; border-top: 1px solid #334155; margin: 25px 0;">

        <div class="toggle-box">
          <span>Tolerância Adaptativa Dinâmica</span>
          <input type="checkbox" id="chkDyn" style="width: auto; transform: scale(1.3);" onchange="updateParametros()">
        </div>

        <div class="slider-container" id="boxTolManual">
          <label>Tolerância Fixa Manual</label>
          <span class="slider-val"><span id="lblTol">150</span> ADC</span>
          <input type="range" id="rngTol" min="10" max="500" value="150" oninput="updateParametros()">
        </div>

        <div class="slider-container" id="boxFatorK">
          <label>Fator de Tolerância K (% da Luz)</label>
          <span class="slider-val"><span id="lblFk">8</span>%</span>
          <input type="range" id="rngFk" min="0.02" max="0.25" step="0.01" value="0.08" oninput="updateParametros()">
        </div>

        <div style="background: #0f172a; padding: 10px; border-radius: 8px; border: 1px solid #334155; margin-bottom: 20px;">
          <span style="color: var(--text-muted); font-size: 0.9rem;">Tolerância Atual Aplicada: </span>
          <b style="color: var(--primary);" id="lblTolEfetiva">--</b>
        </div>

        <div class="slider-container">
          <label>Fator do Filtro EMA (Alfa)</label>
          <span class="slider-val"><span id="lblAlfa">0.15</span></span>
          <input type="range" id="rngAlfa" min="0.01" max="1.0" step="0.01" value="0.15" oninput="updateParametros()">
        </div>

        <div class="slider-container">
          <label>Intervalo de Movimento</label>
          <span class="slider-val"><span id="lblInt">100</span> ms</span>
          <input type="range" id="rngInt" min="10" max="300" step="10" value="100" oninput="updateParametros()">
        </div>

        <hr style="border: 0; border-top: 1px solid #334155; margin: 20px 0;">

        <p style="color: var(--text-muted); font-size: 0.9rem; margin-bottom: 10px;">Offsets Calculados: 
          CE: <b id="offCE">0</b> | CD: <b id="offCD">0</b> | BE: <b id="offBE">0</b> | BD: <b id="offBD">0</b>
        </p>
        
        <button class="btn btn-cal" style="width: 100%;" onclick="executarCalibracaoWeb()">RECALIBRAR OFFSETS DOS LDRS</button>
      </div>
    </div>

    <a href="/logout" class="footer-link">Sair com segurança (Logout)</a>
  </div>

  <script>
    let userEditingParams = false;

    function openTab(tabId, btnElement) {
      document.querySelectorAll('.tab-content').forEach(tab => tab.classList.remove('active'));
      document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
      document.getElementById(tabId).classList.add('active');
      btnElement.classList.add('active');
    }

    const maxDataPoints = 20;

    function criarGrafico(canvasId, cor, minVal=0, maxVal=4095) {
      const ctx = document.getElementById(canvasId).getContext('2d');
      return new Chart(ctx, {
        type: 'line',
        data: {
          labels: Array(maxDataPoints).fill(''),
          datasets: [{
            data: Array(maxDataPoints).fill(0),
            borderColor: cor,
            borderWidth: 2,
            pointRadius: 0,
            tension: 0.3,
            fill: true,
            backgroundColor: cor + '15'
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          plugins: { legend: { display: false } },
          scales: {
            x: { display: false },
            y: { grid: { color: '#334155' }, ticks: { color: '#94a3b8', font: { size: 10 } }, min: minVal, max: maxVal }
          },
          animation: false
        }
      });
    }

    // Gráficos LDR (ADC 0-4095)
    const chartCE = criarGrafico('chartCE', '#38bdf8');
    const chartCD = criarGrafico('chartCD', '#f59e0b');
    const chartBE = criarGrafico('chartBE', '#10b981');
    const chartBD = criarGrafico('chartBD', '#ec4899');

    // Gráficos de Motores (Graus)
    const chartPosH = criarGrafico('chartPosH', '#8b5cf6', 15, 165);
    const chartPosV = criarGrafico('chartPosV', '#ef4444', 0, 90);

    function adicionarDadoGrafico(chart, novoValor, txtElementId) {
      chart.data.datasets[0].data.push(novoValor);
      chart.data.datasets[0].data.shift();
      chart.update();
      if(txtElementId) document.getElementById(txtElementId).innerText = novoValor;
    }

    // Configuração Three.js (Modelo 3D)
    const container = document.getElementById('canvas3d');
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x1a2942);

    const camera = new THREE.PerspectiveCamera(45, container.clientWidth / container.clientHeight, 0.1, 1000);
    camera.position.set(0, 8, 14);
    camera.lookAt(0, 1.5, 0);

    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(container.clientWidth, container.clientHeight);
    renderer.shadowMap.enabled = true;
    renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    container.appendChild(renderer.domElement);

    const planeGeo = new THREE.PlaneGeometry(30, 30);
    const planeMat = new THREE.MeshStandardMaterial({ color: 0x223655, roughness: 0.9 });
    const planeMesh = new THREE.Mesh(planeGeo, planeMat);
    planeMesh.rotation.x = -Math.PI / 2;
    planeMesh.position.y = -1.5;
    planeMesh.receiveShadow = true;
    scene.add(planeMesh);
    
    const gridHelper = new THREE.GridHelper(30, 30, 0x475569, 0x334155);
    gridHelper.position.y = -1.49;
    scene.add(gridHelper);

    scene.add(new THREE.AmbientLight(0xffffff, 0.7));

    const sunGeo = new THREE.SphereGeometry(0.8, 32, 32);
    const sunMat = new THREE.MeshBasicMaterial({ color: 0xffe600 });
    const sunMesh = new THREE.Mesh(sunGeo, sunMat);
    scene.add(sunMesh);

    const sunLight = new THREE.DirectionalLight(0xfffaea, 1.8);
    sunLight.castShadow = true;
    sunLight.shadow.mapSize.width = 2048;
    sunLight.shadow.mapSize.height = 2048;
    scene.add(sunLight);

    const distSol = 9.0;
    const beamGeo = new THREE.CylinderGeometry(3.0, 3.0, distSol, 32, 1, true);
    beamGeo.translate(0, distSol / 2, 0);
    
    const beamMat = new THREE.MeshBasicMaterial({ color: 0xfffaea, transparent: true, opacity: 0.05, blending: THREE.AdditiveBlending, depthWrite: false, side: THREE.DoubleSide });
    const beamMesh = new THREE.Mesh(beamGeo, beamMat);
    scene.add(beamMesh);

    const matChassi = new THREE.MeshStandardMaterial({ color: 0x1e2228, roughness: 0.4, metalness: 0.3 });
    const matDetalhes = new THREE.MeshStandardMaterial({ color: 0x111317, roughness: 0.3, metalness: 0.5 });
    const matPainel = new THREE.MeshStandardMaterial({ color: 0x2b303a, roughness: 0.5, metalness: 0.2 });

    const baseGrupoFixa = new THREE.Group();
    baseGrupoFixa.position.y = -1.5;

    const corpoBaseGeo = new THREE.BoxGeometry(2.2, 0.8, 2.2);
    const corpoBase = new THREE.Mesh(corpoBaseGeo, matChassi);
    corpoBase.position.y = 0.4;
    corpoBase.castShadow = true; corpoBase.receiveShadow = true;
    baseGrupoFixa.add(corpoBase);

    const peGeo = new THREE.CylinderGeometry(0.25, 0.4, 0.5, 16);
    const posPes = [ [-1.1, 0.2, -1.1], [ 1.1, 0.2, -1.1], [-1.1, 0.2,  1.1], [ 1.1, 0.2,  1.1] ];
    posPes.forEach(p => {
      const pe = new THREE.Mesh(peGeo, matChassi);
      pe.position.set(p[0], p[1], p[2]);
      pe.castShadow = true; pe.receiveShadow = true;
      baseGrupoFixa.add(pe);
    });

    const torreBaseGeo = new THREE.CylinderGeometry(0.9, 1.1, 0.8, 32);
    const torreBase = new THREE.Mesh(torreBaseGeo, matChassi);
    torreBase.position.y = 1.2;
    torreBase.castShadow = true; torreBase.receiveShadow = true;
    baseGrupoFixa.add(torreBase);

    scene.add(baseGrupoFixa);

    const grupoAzimutal = new THREE.Group();
    grupoAzimutal.position.y = 0.1;
    scene.add(grupoAzimutal);

    const cabecaRotativaGeo = new THREE.CylinderGeometry(0.85, 0.85, 0.9, 32);
    const cabecaRotativa = new THREE.Mesh(cabecaRotativaGeo, matDetalhes);
    cabecaRotativa.castShadow = true; cabecaRotativa.receiveShadow = true;
    grupoAzimutal.add(cabecaRotativa);

    const bracosLateraisGeo = new THREE.BoxGeometry(1.6, 0.8, 1.0);
    const bracosLaterais = new THREE.Mesh(bracosLateraisGeo, matChassi);
    bracosLaterais.position.y = 0.5;
    bracosLaterais.castShadow = true; bracosLaterais.receiveShadow = true;
    grupoAzimutal.add(bracosLaterais);

    const grupoVertical = new THREE.Group();
    grupoVertical.position.y = 0.9;
    grupoAzimutal.add(grupoVertical);

    const juntaEixoGeo = new THREE.CylinderGeometry(0.35, 0.35, 1.4, 24);
    const juntaEixo = new THREE.Mesh(juntaEixoGeo, matDetalhes);
    juntaEixo.rotation.z = Math.PI / 2;
    juntaEixo.castShadow = true;
    grupoVertical.add(juntaEixo);

    const suporteMesaGeo = new THREE.BoxGeometry(0.8, 0.6, 0.8);
    const suporteMesa = new THREE.Mesh(suporteMesaGeo, matChassi);
    suporteMesa.position.y = 0.3;
    suporteMesa.castShadow = true;
    grupoVertical.add(suporteMesa);

    const placaGrupo = new THREE.Group();
    placaGrupo.position.y = 0.6;
    grupoVertical.add(placaGrupo);

    const shapePlaca = new THREE.Shape();
    shapePlaca.moveTo(-2.2, -1.2); shapePlaca.lineTo( 2.2, -1.2); shapePlaca.lineTo( 3.0, -0.4); shapePlaca.lineTo( 3.0,  0.8);
    shapePlaca.lineTo( 2.0,  1.6); shapePlaca.lineTo(-2.0,  1.6); shapePlaca.lineTo(-3.0,  0.8); shapePlaca.lineTo(-3.0, -0.4);
    shapePlaca.closePath();

    const extrudeSettings = { depth: 0.1, bevelEnabled: true, bevelSegments: 2, steps: 1, bevelSize: 0.03, bevelThickness: 0.03 };
    const placaGeo = new THREE.ExtrudeGeometry(shapePlaca, extrudeSettings);
    const placaMesh = new THREE.Mesh(placaGeo, matPainel);
    placaMesh.rotation.x = Math.PI / 2;
    placaMesh.castShadow = true; placaMesh.receiveShadow = true;
    placaGrupo.add(placaMesh);

    const divisorLdrGeo = new THREE.CylinderGeometry(0.3, 0.4, 0.7, 4);
    const divisorLdr = new THREE.Mesh(divisorLdrGeo, matDetalhes);
    divisorLdr.position.set(0, 0.35, -0.8);
    divisorLdr.rotation.y = Math.PI / 4;
    divisorLdr.castShadow = true;
    placaGrupo.add(divisorLdr);

    function render3D() {
      requestAnimationFrame(render3D);
      renderer.render(scene, camera);
    }
    render3D();

    function atualizarPosicao3D(anguloAzimuteDeg, anguloElevacaoDeg) {
      const radAzimute = (anguloAzimuteDeg - 90) * (Math.PI / 180);
      const radElevacao = (anguloElevacaoDeg - 45) * (Math.PI / 180);
      
      grupoAzimutal.rotation.y = -radAzimute;
      grupoVertical.rotation.x = radElevacao;

      let vetorNormal = new THREE.Vector3(0, 1, 0);
      vetorNormal.applyEuler(new THREE.Euler(radElevacao, 0, 0, 'XYZ'));
      vetorNormal.applyEuler(new THREE.Euler(0, -radAzimute, 0, 'XYZ'));
      
      let direcaoNormal = vetorNormal.clone().normalize();
      vetorNormal.multiplyScalar(distSol);
      
      let origemPlaca = new THREE.Vector3();
      grupoVertical.getWorldPosition(origemPlaca);
      
      sunMesh.position.copy(origemPlaca).add(vetorNormal);
      sunLight.position.copy(sunMesh.position);
      sunLight.target = baseGrupoFixa;

      beamMesh.position.copy(origemPlaca);
      beamMesh.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), direcaoNormal);
    }

    window.addEventListener('resize', () => {
      if(container.clientWidth > 0) {
        camera.aspect = container.clientWidth / container.clientHeight;
        camera.updateProjectionMatrix();
        renderer.setSize(container.clientWidth, container.clientHeight);
      }
    });

    // Função auxiliar para definir classes de status
    function setBadge(elId, inDeadzone) {
      const el = document.getElementById(elId);
      if(inDeadzone) {
        el.className = "badge bg-ok";
        el.innerText = "ALINHADO";
      } else {
        el.className = "badge bg-warn";
        el.innerText = "AJUSTANDO";
      }
    }

    // Comunicação HTTP Constante
    function updateData() {
      fetch('/dados').then(r => r.json()).then(d => {
        // Plotter LDR (Sensores Brutos)
        adicionarDadoGrafico(chartCE, d.ce, 'txtCE');
        adicionarDadoGrafico(chartCD, d.cd, 'txtCD');
        adicionarDadoGrafico(chartBE, d.be, 'txtBE');
        adicionarDadoGrafico(chartBD, d.bd, 'txtBD');
        
        // Plotter Motores (Servos)
        adicionarDadoGrafico(chartPosH, d.posH, 'txtPosH');
        adicionarDadoGrafico(chartPosV, d.posV, 'txtPosV');

        // CÁLCULO DE TELEMETRIA FRONT-END (Descarregando o ESP32)
        let medC = (d.ce + d.cd) / 2;
        let medB = (d.be + d.bd) / 2;
        let medE = (d.ce + d.be) / 2;
        let medD = (d.cd + d.bd) / 2;
        
        let errV = medC - medB;
        let errH = medE - medD;

        document.getElementById('valMedC').innerText = Math.round(medC);
        document.getElementById('valMedB').innerText = Math.round(medB);
        document.getElementById('valMedE').innerText = Math.round(medE);
        document.getElementById('valMedD').innerText = Math.round(medD);

        document.getElementById('valErrV').innerText = Math.round(errV) + " ADC";
        document.getElementById('valErrH').innerText = Math.round(errH) + " ADC";

        // Lógica Visual da Zona Morta (Tolerância)
        setBadge('badgeV', Math.abs(errV) <= d.tolEfetiva || d.manual);
        setBadge('badgeH', Math.abs(errH) <= d.tolEfetiva || d.manual);
        
        // Atualização da Interface
        let txtModo = d.manual ? 'MANUAL' : 'AUTOMÁTICO';
        if (!d.manual) {
           txtModo += d.umEixo ? " (1 EIXO)" : " (2 EIXOS)";
        }
        document.getElementById('modoTxt').innerText = txtModo;

        document.getElementById('btnAuto').className = d.manual ? "btn btn-off" : "btn btn-on";
        document.getElementById('btnMan').className  = d.manual ? "btn btn-on" : "btn btn-off";

        document.getElementById('btn1Eixo').className = d.umEixo ? "btn btn-on" : "btn btn-off";
        document.getElementById('btn2Eixos').className = d.umEixo ? "btn btn-off" : "btn btn-on";

        document.getElementById('lblTolEfetiva').innerText = d.tolEfetiva + " ADC";
        document.getElementById('offCE').innerText = d.offCE;
        document.getElementById('offCD').innerText = d.offCD;
        document.getElementById('offBE').innerText = d.offBE;
        document.getElementById('offBD').innerText = d.offBD;

        if (!userEditingParams) {
          document.getElementById('chkDyn').checked = d.dynTol == 1;
          document.getElementById('rngTol').value = d.tol;
          document.getElementById('lblTol').innerText = d.tol;
          document.getElementById('rngFk').value = d.fatorK;
          document.getElementById('lblFk').innerText = Math.round(d.fatorK * 100);
          document.getElementById('rngAlfa').value = d.alfa;
          document.getElementById('lblAlfa').innerText = d.alfa;
          document.getElementById('rngInt').value = d.intervalo;
          document.getElementById('lblInt').innerText = d.intervalo;
          
          if(document.activeElement.id !== "valKp") document.getElementById('valKp').value = d.kp;
          if(document.activeElement.id !== "valKi") document.getElementById('valKi').value = d.ki;
          if(document.activeElement.id !== "valKd") document.getElementById('valKd').value = d.kd;
        }

        atualizarPosicao3D(d.posH, d.posV);
      }).catch(e => console.log("Aguardando comunicacao..."));
    }

    function setModo(m) { fetch('/setModo?manual=' + m); setTimeout(updateData, 100); }
    function setEixos(e) { fetch('/setEixos?um=' + e); setTimeout(updateData, 100); }

    // ==========================================
    // NOVO: LÓGICA DE THROTTLING PARA OS SLIDERS
    // ==========================================
    let timerEnvio = null;

    function updateSliders() {
      let h = document.getElementById('rngH').value;
      let v = document.getElementById('rngV').value;
      
      // Atualiza os números na interface imediatamente
      document.getElementById('lblH').innerText = h;
      document.getElementById('lblV').innerText = v;
      
      // Se já houver um envio pendente, ignora este evento para não sobrecarregar
      if (timerEnvio !== null) return;
      
      // Cria um atraso de 100ms antes de permitir um novo envio ao ESP32
      timerEnvio = setTimeout(() => {
        fetch(`/controlManual?h=${h}&v=${v}`);
        timerEnvio = null; // Liberta o bloqueio para o próximo movimento
      }, 100);
    }

    function updateParametros() {
      userEditingParams = true;
      let dyn = document.getElementById('chkDyn').checked ? 1 : 0;
      let tol = document.getElementById('rngTol').value;
      let fk = document.getElementById('rngFk').value;
      let alfa = document.getElementById('rngAlfa').value;
      let intervalo = document.getElementById('rngInt').value;

      document.getElementById('lblTol').innerText = tol;
      document.getElementById('lblFk').innerText = Math.round(fk * 100);
      document.getElementById('lblAlfa').innerText = alfa;
      document.getElementById('lblInt').innerText = intervalo;

      fetch(`/setParametros?dyn=${dyn}&tol=${tol}&fk=${fk}&alfa=${alfa}&int=${intervalo}`);
      setTimeout(() => { userEditingParams = false; }, 2000);
    }
    
    function updatePID() {
      let kp = document.getElementById('valKp').value;
      let ki = document.getElementById('valKi').value;
      let kd = document.getElementById('valKd').value;
      fetch(`/setPID?kp=${kp}&ki=${ki}&kd=${kd}`);
    }

    function executarCalibracaoWeb() {
      fetch('/calibrar');
      alert("Calibração iniciada! Mantenha a iluminação constante sobre a maquete.");
    }

    setInterval(updateData, 200);
  </script>
</body>
</html>
)rawliteral";

// ==========================================
// RESPOSTA DE DADOS HTTP (API JSON OTIMIZADA)
// ==========================================
inline void enviarDadosJSON() {
  char jsonBuffer[512];
  snprintf(jsonBuffer, sizeof(jsonBuffer), 
    "{\"ce\":%d,\"cd\":%d,\"be\":%d,\"bd\":%d,\"posH\":%d,\"posV\":%d,"
    "\"manual\":%d,\"umEixo\":%d,\"dynTol\":%d,\"tol\":%d,\"fatorK\":%.2f,"
    "\"tolEfetiva\":%d,\"alfa\":%.2f,\"intervalo\":%ld,\"offCE\":%d,"
    "\"offCD\":%d,\"offBE\":%d,\"offBD\":%d,\"kp\":%.4f,\"ki\":%.4f,\"kd\":%.4f}",
    valCE, valCD, valBE, valBD, posicaoAzimutal, posicaoVertical,
    modoManualAtivo ? 1 : 0, modoUmEixo ? 1 : 0, usarToleranciaDinamica ? 1 : 0,
    toleranciaLuz, fatorK, toleranciaEfetiva, alfaFiltro, intervaloMovimento,
    offsetCE, offsetCD, offsetBE, offsetBD, Kp, Ki, Kd);

  server.send(200, "application/json", jsonBuffer);
}

#endif // WEB_SERVER_H