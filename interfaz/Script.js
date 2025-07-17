/**
 * Script.js - Versión con tabla de condiciones GPIO mejorada
 */
document.addEventListener('DOMContentLoaded', () => {
const $ = id => document.getElementById(id);

// Estado global
let sistema = {
pantalla: null,
wifi: { ssid: '', password: '', connected: false, availableNetworks: [], savedNetworks: [] },
datos: { id: '', canal: '' },
uart: { baudrate: 115200, dataBits: 8, parity: 'none', stopBits: 1, enabled: false },
i2c: { clockSpeed: 100, sdaPin: 21, sclPin: 22, enabled: false },
gpio: { condiciones: [] }
};
let currentSsidToConnect = '';
let isUpdateMode = false;

// 1. Toast y persistencia
function showToast(msg, type = 'info') {
let toastContainer = $('toastContainer');
if (!toastContainer) {
toastContainer = document.createElement('div');
toastContainer.id = 'toastContainer';
toastContainer.style.position = 'fixed';
toastContainer.style.bottom = '20px';
toastContainer.style.right = '20px';
toastContainer.style.zIndex = '10000';
toastContainer.style.display = 'flex';
toastContainer.style.flexDirection = 'column';
toastContainer.style.gap = '10px';
document.body.appendChild(toastContainer);
}
const toast = document.createElement('div');
toast.className = `toast ${type}`;
toast.textContent = msg;
toast.style.backgroundColor = type === 'success' ? '#28a745'
: type === 'error' ? '#dc3545'
: type === 'info' ? '#17a2b8'
: '#333';
toast.style.color = 'white';
toast.style.padding = '12px 20px';
toast.style.borderRadius = '5px';
toast.style.boxShadow = '0 3px 10px rgba(0,0,0,0.2)';
toast.style.opacity = '1';
toast.style.transition = 'opacity 0.5s, transform 0.5s';
toast.style.minWidth = '250px';
toast.style.textAlign = 'center';
toastContainer.appendChild(toast);
setTimeout(() => {
toast.style.opacity = '0';
toast.addEventListener('transitionend', () => toast.remove());
}, 3000);
}

function guardarEstado() {
localStorage.setItem('sistema', JSON.stringify(sistema));
}

function cargarEstado() {
try {
const saved = JSON.parse(localStorage.getItem('sistema'));
if (saved) {
sistema.pantalla = saved.pantalla !== undefined ? saved.pantalla : sistema.pantalla;
sistema.wifi = saved.wifi || sistema.wifi;
sistema.datos = saved.datos || sistema.datos;
sistema.uart = saved.uart || sistema.uart;
sistema.i2c = saved.i2c || sistema.i2c;
sistema.gpio = saved.gpio || sistema.gpio;
}
} catch (e) {
console.error('Error cargando estado:', e);
}
}

// 2. Modal WiFi
function showPasswordModal(ssid, isUpdate = false) {
currentSsidToConnect = ssid;
isUpdateMode = isUpdate;
$('modalSsidName').textContent = isUpdate ? `Actualizar "${ssid}"` : `Contraseña para "${ssid}"`;
$('wifiPasswordInput').value = '';
$('wifiPasswordError').textContent = '';
$('wifiPasswordError').style.display = 'none';
$('wifiPasswordInput').classList.remove('error');
$('wifiPasswordModal').classList.add('active');
}

function hidePasswordModal() {
$('wifiPasswordModal').classList.remove('active');
}

$('connectWifiButton').onclick = function() {
const password = $('wifiPasswordInput').value.trim();
if (!password) {
$('wifiPasswordError').textContent = 'La contraseña no puede estar vacía.';
$('wifiPasswordError').style.display = 'block';
$('wifiPasswordInput').classList.add('error');
$('wifiPasswordInput').focus();
return;
}
$('wifiPasswordInput').classList.remove('error');
if (isUpdateMode) {
actualizarRedConValidacion(currentSsidToConnect, password);
} else {
guardarRedConValidacion(currentSsidToConnect, password);
}
};

$('cancelWifiButton').onclick = hidePasswordModal;

// 3. Render y lógica principal
function render(subsection) {
actualizarResumenInicio();

if (subsection === 'pantalla') {
$('funciones-content').innerHTML = `
<div class="pantalla-status-box">
<h4>Control de Pantalla</h4>
<div class="pantalla-status-row">
<span>Estado actual: <strong id="pantallaSubEstado">${sistema.pantalla === null ? '----' : (sistema.pantalla ? 'Encendida' : 'Apagada')}</strong></span>
</div>
<div class="pantalla-status-actions">
<button id="pantallaSubEncender" class="action-button success-button"><i class="fas fa-power-off"></i> Encender</button>
<button id="pantallaSubApagar" class="action-button danger-button"><i class="fas fa-power-off"></i> Apagar</button>
</div>
</div>`;

$('pantallaSubEncender').onclick = () => {
fetch('/pantalla/on').then(res => res.json()).then(() => {
sistema.pantalla = true; 
guardarEstado(); 
render('pantalla'); 
showToast('Pantalla encendida', 'success');
actualizarResumenInicio();
}).catch(() => showToast('Error al encender pantalla', 'error'));
};

$('pantallaSubApagar').onclick = () => {
fetch('/pantalla/off').then(res => res.json()).then(() => {
sistema.pantalla = false; 
guardarEstado(); 
render('pantalla'); 
showToast('Pantalla apagada', 'success');
actualizarResumenInicio();
}).catch(() => showToast('Error al apagar pantalla', 'error'));
};

} else if (subsection === 'wifi') {
$('funciones-content').innerHTML = `
<div class="wifi-sub-section">
<h4>Configuración de WiFi</h4>
<div class="wifi-options-buttons">
<button id="showAvailableNetworksBtn" class="action-button active">Redes Disponibles</button>
<button id="showSavedNetworksBtn" class="action-button">Redes Guardadas</button>
</div>
<div id="availableNetworksSection" class="wifi-section-content active-content">
<div class="action-buttons-container">
<button id="showAvailableNetworksListBtn" class="action-button success-button">Mostrar Redes Disponibles</button>
</div>
<div class="wifi-table-container">
<table class="wifi-table" id="availableNetworksTable">
<thead>
<tr>
<th>SSID</th>
<th>Seguridad</th>
<th>Intensidad</th>
<th>Acciones</th>
</tr>
</thead>
<tbody id="availableNetworksTableBody">
<tr>
<td colspan="4" class="no-data-message">Presione el botón para Obtener las Redes Disponibles</td>
</tr>
</tbody>
</table>
</div>
<p id="noAvailableNetworks" class="no-data-message" style="display:none;">No se encontraron redes disponibles.</p>
</div>
<div id="savedNetworksSection" class="wifi-section-content">
<div class="action-buttons-container">
<button id="showSavedNetworksListBtn" class="action-button success-button">Mostrar Redes Guardadas</button>
<button id="clearAllSavedNetworksBtn" class="action-button danger-button">Borrar Todas las Redes Guardadas</button>
</div>
<div class="wifi-table-container">
<table class="wifi-table" id="savedNetworksTable">
<thead>
<tr>
<th>Nombre Red</th>
<th>Acciones</th>
</tr>
</thead>
<tbody id="savedNetworksTableBody">
<tr>
<td colspan="2" class="no-data-message">Presione el botón para Obtener las Redes Guardadas</td>
</tr>
</tbody>
</table>
</div>
<p id="noSavedNetworks" class="no-data-message" style="display:none;">No hay redes guardadas.</p>
</div>
</div>`;
setupWifiSection();

} else if (subsection === 'uart') {
$('funciones-content').innerHTML = `
<div class="config-section">
<div class="config-box">
<h3><i class="fas fa-microchip"></i> Configuración UART</h3>
<div class="config-actions">
<button id="enableUartBtn" class="action-button ${sistema.uart.enabled ? 'active-button' : 'success-button'}">Habilitar UART</button>
<button id="disableUartBtn" class="action-button ${!sistema.uart.enabled ? 'active-button' : 'danger-button'}">Deshabilitar UART</button>
</div>
</div>
</div>`;

// Event listeners para UART
$('enableUartBtn').onclick = () => {
fetch('/enableUart', {
method: 'POST',
headers: {'Content-Type': 'application/json'}
})
.then(res => res.json())
.then(data => {
if (data.success) {
sistema.uart.enabled = true;
$('enableUartBtn').classList.add('active-button');
$('enableUartBtn').classList.remove('success-button');
$('disableUartBtn').classList.remove('active-button');
$('disableUartBtn').classList.add('danger-button');
showToast('UART habilitado correctamente', 'success');
guardarEstado();
} else {
showToast('Error al habilitar UART', 'error');
}
})
.catch(() => showToast('Error de conexión', 'error'));
};

$('disableUartBtn').onclick = () => {
fetch('/disableUart', {
method: 'POST',
headers: {'Content-Type': 'application/json'}
})
.then(res => res.json())
.then(data => {
if (data.success) {
sistema.uart.enabled = false;
$('disableUartBtn').classList.add('active-button');
$('disableUartBtn').classList.remove('danger-button');
$('enableUartBtn').classList.remove('active-button');
$('enableUartBtn').classList.add('success-button');
showToast('UART deshabilitado correctamente', 'success');
guardarEstado();
} else {
showToast('Error al deshabilitar UART', 'error');
}
})
.catch(() => showToast('Error de conexión', 'error'));
};

} else if (subsection === 'i2c') {
$('funciones-content').innerHTML = `
<div class="config-section">
<div class="config-box">
<h3><i class="fas fa-microchip"></i> Configuración I2C</h3>
<div class="config-actions">
<button id="enableI2cBtn" class="action-button ${sistema.i2c.enabled ? 'active-button' : 'success-button'}">Habilitar I2C</button>
<button id="disableI2cBtn" class="action-button ${!sistema.i2c.enabled ? 'active-button' : 'danger-button'}">Deshabilitar I2C</button>
<button id="testI2cBtn" class="action-button info-button" ${!sistema.i2c.enabled ? 'disabled' : ''}>
<i class="fas fa-vial"></i> Test I2C
</button>
</div>
</div>
</div>`;

// Event listeners para I2C
$('enableI2cBtn').onclick = () => {
fetch('/enableI2c', {
method: 'POST',
headers: {'Content-Type': 'application/json'}
})
.then(res => res.json())
.then(data => {
if (data.success) {
sistema.i2c.enabled = true;
$('enableI2cBtn').classList.add('active-button');
$('enableI2cBtn').classList.remove('success-button');
$('disableI2cBtn').classList.remove('active-button');
$('disableI2cBtn').classList.add('danger-button');
$('testI2cBtn').disabled = false;
showToast('I2C habilitado correctamente', 'success');
guardarEstado();
} else {
showToast('Error al habilitar I2C', 'error');
}
})
.catch(() => showToast('Error de conexión', 'error'));
};

$('disableI2cBtn').onclick = () => {
fetch('/disableI2c', {
method: 'POST',
headers: {'Content-Type': 'application/json'}
})
.then(res => res.json())
.then(data => {
if (data.success) {
sistema.i2c.enabled = false;
$('disableI2cBtn').classList.add('active-button');
$('disableI2cBtn').classList.remove('danger-button');
$('enableI2cBtn').classList.remove('active-button');
$('enableI2cBtn').classList.add('success-button');
$('testI2cBtn').disabled = true;
showToast('I2C deshabilitado correctamente', 'success');
guardarEstado();
} else {
showToast('Error al deshabilitar I2C', 'error');
}
})
.catch(() => showToast('Error de conexión', 'error'));
};

$('testI2cBtn').onclick = () => {
showToast('Iniciando pruebas I2C...', 'info');
fetch('/testI2c')
.then(res => res.json())
.then(data => {
if (data.success) {
showToast('Pruebas I2C completadas: ' + data.message, 'success');
} else {
showToast('Error en pruebas I2C: ' + data.message, 'error');
}
})
.catch(() => showToast('Error al realizar pruebas I2C', 'error'));
};

} else if (subsection === 'gpio') {
$('funciones-content').innerHTML = `
<div class="gpio-main-content">
<div class="gpio-white-box">
<h3>Configuración GPIO</h3>
<p>Seleccione una opción del menú lateral para gestionar los GPIO</p>
</div>
</div>`;

// Mostrar submenú GPIO
$('gpioSubmenu').classList.add('active');
$('submenuOverlay').style.display = 'block';

} else if (subsection === 'gpio-archivos') {
$('funciones-content').innerHTML = `
<div class="gpio-file-section">
<h4>Archivos GPIO</h4>
<div class="file-actions">
<div class="file-action-group">
<button id="downloadGpioConfigBtn" class="action-button success-button">
<i class="fas fa-download"></i> Descargar Configuración
</button>
<p class="file-action-description">Descargar la configuración actual en formato JSON</p>
</div>
<div class="file-action-group">
<button id="uploadGpioConfigBtn" class="action-button info-button">
<i class="fas fa-upload"></i> Subir Configuración
</button>
<p class="file-action-description">Subir un archivo JSON para actualizar la configuración</p>
</div>
<input type="file" id="gpioConfigFileInput" accept=".json" style="display: none;">
</div>
<div id="gpioFileStatus" class="file-status"></div>
</div>`;

$('downloadGpioConfigBtn').onclick = downloadGpioConfig;
$('uploadGpioConfigBtn').onclick = () => $('gpioConfigFileInput').click();

$('gpioConfigFileInput').onchange = function(e) {
const file = e.target.files[0];
if (!file) return;

const fileStatus = $('gpioFileStatus');
fileStatus.textContent = `Archivo seleccionado: ${file.name}`;
fileStatus.style.color = '#17a2b8';

if (file.type !== "application/json" && !file.name.endsWith('.json')) {
fileStatus.textContent = 'Error: El archivo debe ser un JSON válido';
fileStatus.style.color = '#dc3545';
return;
}

const reader = new FileReader();
reader.onload = function(e) {
try {
const jsonData = JSON.parse(e.target.result);
fileStatus.textContent = 'Procesando archivo...';
uploadGpioConfig(jsonData);
} catch (error) {
fileStatus.textContent = 'Error: El archivo JSON no es válido';
fileStatus.style.color = '#dc3545';
console.error('Error parsing JSON:', error);
}
};
reader.onerror = function() {
fileStatus.textContent = 'Error al leer el archivo';
fileStatus.style.color = '#dc3545';
};
reader.readAsText(file);
};

} else if (subsection === 'gpio-condicion') {
$('funciones-content').innerHTML = `
<div class="gpio-condicion-section">
<h4>Matriz de Condiciones GPIO</h4>

<div class="gpio-matrix-container">
<div class="gpio-matrix">
<!-- Cabecera con letras -->
<div class="matrix-header">
<div class="matrix-corner">+\\*</div>
${['A','B','C','D','E','F'].map(letter => `
<div class="matrix-header-cell">${letter}</div>
`).join('')}
</div>

<!-- Filas con números y letras -->
<div class="matrix-body">
${Array.from({length: 12}, (_, row) => {
const rowNum = row + 1;
const outputLetter = String.fromCharCode(79 + row); // O-Z
return `
<div class="matrix-row">
<!-- Número de fila -->
<div class="matrix-row-header">${rowNum}</div>

<!-- Celdas de condición -->
${['A','B','C','D','E','F'].map((letter, col) => {
const cellId = `${rowNum}${letter}`;
// Si es la celda 4D, no mostrar nada
if (cellId === '4D') return `
<div class="matrix-cell" data-row="4" data-col="D" data-id="4D">
<span class="cell-content"></span>
</div>
`;
const existingCond = sistema.gpio.condiciones.find(
cond => cond.comando.includes(cellId)
);
const cellContent = existingCond ? 
`${existingCond.pin}${existingCond.tipo}${existingCond.direccion}${existingCond.id}` : '';
return `
<div class="matrix-cell ${existingCond ? 'active' : ''}" 
data-row="${rowNum}" 
data-col="${letter}"
data-id="${cellId}">
<span class="cell-content">${cellContent}</span>
</div>
`;
}).join('')}                                       
<!-- Letra de salida -->
<div class="matrix-row-footer">
${outputLetter}
</div>
</div>
`;
}).join('')}
</div>
</div>

</div>
</div>
</div>`;

// Función para actualizar la tabla de condiciones
function updateConditionsTable() {
const tbody = $('gpioConditionsBody');
tbody.innerHTML = sistema.gpio.condiciones.length > 0 ? 
sistema.gpio.condiciones.map(cond => `
<tr data-command="${cond.comando}">
<td>${cond.comando}</td>
<td>${cond.pin}</td>
<td>${cond.tipo === 'E' ? 'Entrada' : 'Salida'}</td>
<td>${cond.direccion === 'A' ? 'Ascendente' : 'Descendente'}</td>
<td>${cond.id}</td>
<td>
<button class="action-button danger-button delete-cond" 
data-command="${cond.comando}">
<i class="fas fa-trash"></i>
</button>
</td>
</tr>
`).join('') : `
<tr>
<td colspan="6" class="no-conditions">No hay condiciones configuradas</td>
</tr>`;

// Agregar event listeners a los botones de eliminar
document.querySelectorAll('.delete-cond').forEach(btn => {
btn.addEventListener('click', function() {
const command = this.getAttribute('data-command');
if (confirm(`¿Eliminar la condición "${command}"?`)) {
sistema.gpio.condiciones = sistema.gpio.condiciones.filter(
cond => cond.comando !== command
);
guardarEstado();
updateConditionsTable();
updateMatrixCells();
showToast('Condición eliminada', 'success');
}
});
});
}

// Función para actualizar las celdas de la matriz


// Event listener para las celdas de la matriz
document.querySelectorAll('.matrix-cell').forEach(cell => {
cell.addEventListener('click', function() {
// Quitar selección de todas las celdas
document.querySelectorAll('.matrix-cell').forEach(c => {
c.classList.remove('selected');
});

// Seleccionar la celda actual
this.classList.add('selected');

const cellId = this.getAttribute('data-id');
const row = cellId[0];
const col = cellId[1];

// Autorellenar los campos con los valores de la celda
$('gpioCondId').value = row;

// Buscar si ya existe una condición para esta celda
const existingCond = sistema.gpio.condiciones.find(
cond => cond.comando.includes(cellId)
);

if (existingCond) {
$('gpioPinSelect').value = existingCond.pin;
$('gpioTypeSelect').value = existingCond.tipo;
$('gpioDirSelect').value = existingCond.direccion;
$('gpioCondId').value = existingCond.id;
}
});
});

// Event listener para guardar condición
$('saveGpioCondBtn').addEventListener('click', function() {
const pin = $('gpioPinSelect').value;
const tipo = $('gpioTypeSelect').value;
const direccion = $('gpioDirSelect').value;
const id = $('gpioCondId').value;

if (!id || !/^\d{1,2}$/.test(id) || parseInt(id) < 1 || parseInt(id) > 12) {
showToast('Ingrese un ID válido (1-12)', 'error');
return;
}

// Obtener la celda seleccionada
const selectedCell = document.querySelector('.matrix-cell.selected');
if (!selectedCell) {
showToast('Seleccione una celda en la matriz primero', 'error');
return;
}

const cellId = selectedCell.getAttribute('data-id');
const comando = `${cellId}${pin}${tipo}${direccion}${id}`;

// Eliminar cualquier condición existente para esta celda
sistema.gpio.condiciones = sistema.gpio.condiciones.filter(
cond => !cond.comando.includes(cellId)
);

// Agregar la nueva condición
sistema.gpio.condiciones.push({
comando,
pin,
tipo,
direccion,
id,
celda: cellId
});

guardarEstado();
updateConditionsTable();
updateMatrixCells();
showToast('Condición guardada correctamente', 'success');

// Deseleccionar la celda
selectedCell.classList.remove('selected');
});

// Event listener para limpiar todo
$('clearGpioCondBtn').addEventListener('click', function() {
if (confirm('¿Está seguro que desea eliminar todas las condiciones?')) {
sistema.gpio.condiciones = [];
guardarEstado();
updateConditionsTable();
updateMatrixCells();
showToast('Todas las condiciones han sido eliminadas', 'success');

// Deseleccionar cualquier celda seleccionada
document.querySelectorAll('.matrix-cell').forEach(c => {
c.classList.remove('selected');
});

// Limpiar campos
$('gpioCondId').value = '';
}
});

// Inicializar la tabla y la matriz
updateConditionsTable();
updateMatrixCells();

} else if (subsection === 'inicio') {
actualizarResumenInicio();
}
}

function actualizarResumenInicio() {
$('perro_diecinueve').textContent = sistema.pantalla === null ? '----' : (sistema.pantalla ? 'ENCENDIDA' : 'APAGADA');
$('perro_veintiuno').textContent = sistema.wifi.connected ? `Conectado a: ${sistema.wifi.ssid}` : '----';
$('perro_veintiuno').style.color = sistema.wifi.connected ? '#2ecc71' : '#e74c3c';
$('perro_id_value').textContent = sistema.datos.id || '---';
$('perro_canal_value').textContent = sistema.datos.canal || '---';
}

// 4. Funciones para GPIO
function downloadGpioConfig() {
showToast('Descargando configuración GPIO...', 'info');
fetch('/getGpioConfig')
.then(response => response.json())
.then(data => {
const dataStr = JSON.stringify(data, null, 2);
const dataBlob = new Blob([dataStr], { type: 'application/json' });
const url = URL.createObjectURL(dataBlob);
const a = document.createElement('a');
a.href = url;
a.download = 'gpio_config.json';
document.body.appendChild(a);
a.click();
setTimeout(() => {
document.body.removeChild(a);
URL.revokeObjectURL(url);
}, 0);
showToast('Configuración descargada correctamente', 'success');
$('gpioFileStatus').textContent = 'Configuración descargada con éxito';
$('gpioFileStatus').style.color = '#28a745';
})
.catch(error => {
console.error('Error downloading GPIO config:', error);
showToast('Error al descargar configuración', 'error');
$('gpioFileStatus').textContent = 'Error al descargar configuración';
$('gpioFileStatus').style.color = '#dc3545';
});
}

function uploadGpioConfig(config) {
showToast('Subiendo configuración GPIO...', 'info');
fetch('/setGpioConfig', {
method: 'POST',
headers: { 'Content-Type': 'application/json' },
body: JSON.stringify(config)
})
.then(response => response.json())
.then(data => {
if (data.success) {
showToast('Configuración GPIO actualizada correctamente', 'success');
$('gpioFileStatus').textContent = 'Configuración actualizada con éxito';
$('gpioFileStatus').style.color = '#28a745';
} else {
showToast('Error al actualizar configuración GPIO', 'error');
$('gpioFileStatus').textContent = 'Error al actualizar configuración';
$('gpioFileStatus').style.color = '#dc3545';
}
})
.catch(error => {
console.error('Error uploading GPIO config:', error);
showToast('Error al subir configuración', 'error');
$('gpioFileStatus').textContent = 'Error al subir configuración';
$('gpioFileStatus').style.color = '#dc3545';
});
}

// 5. Lógica WiFi
/**
* Guarda una red WiFi, pero solo si hay menos de 3 redes guardadas.
*/
function guardarRedConValidacion(ssid, password) {
fetch('/redesGuardadas')
.then(res => res.json())
.then(data => {
const networks = data.networks || [];
if (networks.length >= 3) {
showToast('Solo puedes guardar hasta 3 redes WiFi.', 'error');
hidePasswordModal();
return;
}
showToast('Guardando red...', 'info');
fetch('/guardarRed', {
method: 'POST',
headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
})
.then(res => res.json())
.then(data => {
if (data.mensaje) {
showToast('Red guardada correctamente', 'success');
sistema.wifi.ssid = ssid;
sistema.wifi.connected = true;
guardarEstado();
actualizarResumenInicio();
obtenerRedesGuardadas();
hidePasswordModal();
// Ya no se reinicia aquí
} else {
showToast('Error al guardar la red', 'error');
}
})
.catch(() => showToast('Error al guardar la red', 'error'));
})
.catch(() => {
showToast('Error al verificar redes guardadas', 'error');
});
}
function actualizarRedConValidacion(ssid, password) {
showToast('Actualizando red...', 'info');
//Actualizar una red
fetch('/actualizarRed', {
method: 'POST',
headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
})
.then(res => res.json())
.then(data => {
if (data.mensaje) {
showToast('Red actualizada correctamente', 'success');
obtenerRedesGuardadas();
hidePasswordModal();
} else {
showToast('Error al actualizar la red', 'error');
}
})
.catch(() => showToast('Error al actualizar la red', 'error'));
}

function conectarRed(ssid) {
showToast('Conectando...', 'info');
//Conectar la una red 
fetch('/conectarRed', {
method: 'POST',
headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
body: `ssid=${encodeURIComponent(ssid)}`
})
.then(res => res.json())
.then(data => {
showToast('Configuración guardada. Reconectando...', 'success');
sistema.wifi.ssid = ssid;
sistema.wifi.connected = true;
guardarEstado();
actualizarResumenInicio();
// Ya no se reinicia aquí
})
.catch(() => showToast('Error al conectar a la red', 'error'));
}

function obtenerRedesGuardadas() {
const savedTableBody = document.getElementById('savedNetworksTableBody');
const noSavedMsg = document.getElementById('noSavedNetworks');
if (savedTableBody) {
savedTableBody.innerHTML = `<tr><td colspan="2" class="no-data-message">Cargando redes guardadas...</td></tr>`;
}

//Guardar una red wifi
fetch('/redesGuardadas')
.then(res => res.json())
.then(data => {
const networks = data.networks || [];
if (networks.length === 0) {
if (savedTableBody) savedTableBody.innerHTML = '';
if (noSavedMsg) noSavedMsg.style.display = 'block';
} else {
if (noSavedMsg) noSavedMsg.style.display = 'none';
if (savedTableBody) savedTableBody.innerHTML = '';

networks.forEach(net => {
const tr = document.createElement('tr');
tr.innerHTML = `
<td>${net.ssid}</td>
<td>
<button class="action-button success-button" data-action="connect" data-ssid="${net.ssid}">Conectar</button>
<button class="action-button info-button" data-action="update" data-ssid="${net.ssid}">Actualizar</button>
<button class="action-button danger-button" data-action="delete" data-ssid="${net.ssid}">Eliminar</button>
</td>
`;
savedTableBody.appendChild(tr);
});

savedTableBody.querySelectorAll('button[data-action]').forEach(btn => {
const action = btn.getAttribute('data-action');
const ssid = btn.getAttribute('data-ssid');
btn.onclick = function() {
if (action === 'connect') {
conectarRed(ssid);
} else if (action === 'update') {
showPasswordModal(ssid, true);
} else if (action === 'delete') {
if (confirm(`¿Eliminar la red "${ssid}"?`)) {
fetch('/borrarRed', {
method: 'POST',
headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
body: `ssid=${encodeURIComponent(ssid)}`
})
.then(res => res.json())
.then(data => {
showToast('Red eliminada', 'success');
obtenerRedesGuardadas();
})
.catch(() => showToast('Error al eliminar la red', 'error'));
}
}
};
});
}
})
.catch(() => {
if (savedTableBody) savedTableBody.innerHTML = `<tr><td colspan="2" class="no-data-message">Error al cargar redes guardadas</td></tr>`;
if (noSavedMsg) noSavedMsg.style.display = 'none';
});
}

function setupWifiSection() {
const showAvailableBtn = $('showAvailableNetworksBtn');
const showSavedBtn = $('showSavedNetworksBtn');
const availableSection = $('availableNetworksSection');
const savedSection = $('savedNetworksSection');
const showAvailableListBtn = $('showAvailableNetworksListBtn');
const availableTableBody = $('availableNetworksTableBody');
const noAvailableMsg = $('noAvailableNetworks');
const showSavedListBtn = $('showSavedNetworksListBtn');
const clearAllSavedBtn = $('clearAllSavedNetworksBtn');

showAvailableBtn.onclick = () => {
showAvailableBtn.classList.add('active');
showSavedBtn.classList.remove('active');
availableSection.classList.add('active-content');
savedSection.classList.remove('active-content');
};

showSavedBtn.onclick = () => {
showSavedBtn.classList.add('active');
showAvailableBtn.classList.remove('active');
savedSection.classList.add('active-content');
availableSection.classList.remove('active-content');
};

showAvailableListBtn.onclick = function() {
availableTableBody.innerHTML = `<tr><td colspan="4" class="no-data-message">Buscando redes WiFi...</td></tr>`;
noAvailableMsg.style.display = 'none';

//Escaneo de las redes disponibles 
fetch('/redesDisponibles')
.then(res => res.json())
.then(data => {
const networks = data.networks || [];
if (networks.length === 0) {
availableTableBody.innerHTML = '';
noAvailableMsg.style.display = 'block';
} else {
noAvailableMsg.style.display = 'none';
availableTableBody.innerHTML = '';
networks.forEach(net => {
const tr = document.createElement('tr');
tr.innerHTML = `
<td>${net.ssid}</td>
<td>${net.security}</td>
<td>
<span class="wifi-signal signal-${net.intensity}">
${'&#9679;'.repeat(net.intensity)}
</span>
</td>
<td>
<button class="action-button success-button" data-ssid="${net.ssid}">Conectar</button>
</td>
`;
availableTableBody.appendChild(tr);
});

availableTableBody.querySelectorAll('button[data-ssid]').forEach(btn => {
btn.onclick = function() {
const ssid = this.getAttribute('data-ssid');
showPasswordModal(ssid, false);
};
});
}
})
.catch(err => {
availableTableBody.innerHTML = `<tr><td colspan="4" class="no-data-message">Error al buscar redes WiFi</td></tr>`;
noAvailableMsg.style.display = 'none';
showToast('Error al buscar redes WiFi', 'error');
});
};

if (showSavedListBtn) showSavedListBtn.onclick = obtenerRedesGuardadas;

if (clearAllSavedBtn) {
clearAllSavedBtn.onclick = function() {
if (confirm('¿Borrar todas las redes guardadas?')) {
fetch('/borrarRedes')
.then(res => res.json())
.then(() => {
showToast('Todas las redes guardadas eliminadas', 'success');
obtenerRedesGuardadas();
})
.catch(() => showToast('Error al borrar redes guardadas', 'error'));
}
};
}
}

// 6. Navegación y datos
function setupNavigationYDatos() {
const sidebar = $('perro_cinco');
const menuToggle = $('perro_dos');
const mainContent = $('perro_doce');
const navLinks = document.querySelectorAll('#perro_siete ul li a');
const pantallaSubmenu = $('pantallaSubmenu');
const gpioSubmenu = $('gpioSubmenu');
const pantallaSubmenuLinks = pantallaSubmenu.querySelectorAll('li a[data-subsection]');
const gpioSubmenuLinks = gpioSubmenu.querySelectorAll('li a[data-subsection]');
const FUNCIONES_SIDEBAR_WIDTH = 150;

let submenuOverlay = document.getElementById('submenuOverlay');
if (!submenuOverlay) {
submenuOverlay = document.createElement('div');
submenuOverlay.id = 'submenuOverlay';
submenuOverlay.className = 'submenu-overlay';
submenuOverlay.style.position = 'fixed';
submenuOverlay.style.top = '0';
submenuOverlay.style.left = '0';
submenuOverlay.style.width = '100vw';
submenuOverlay.style.height = '100vh';
submenuOverlay.style.background = 'rgba(0,0,0,0.15)';
submenuOverlay.style.zIndex = '100';
submenuOverlay.style.display = 'none';
document.body.appendChild(submenuOverlay);
}

function closeAllSubmenus() {
sidebar.classList.remove('active');
pantallaSubmenu.classList.remove('active');
gpioSubmenu.classList.remove('active');
mainContent.classList.remove('shifted');
mainContent.style.marginLeft = '0';
menuToggle.classList.remove('active');
submenuOverlay.style.display = 'none';
}

menuToggle.onclick = () => {
sidebar.classList.toggle('active');
mainContent.classList.toggle('shifted');
mainContent.style.marginLeft = sidebar.classList.contains('active') ? FUNCIONES_SIDEBAR_WIDTH + 'px' : '0';
menuToggle.classList.toggle('active');
if (sidebar.classList.contains('active')) {
submenuOverlay.style.display = 'block';
} else {
closeAllSubmenus();
}
};

submenuOverlay.onclick = closeAllSubmenus;
submenuOverlay.addEventListener('touchstart', closeAllSubmenus);

mainContent.addEventListener('click', function(e) {
if (pantallaSubmenu.classList.contains('active') || gpioSubmenu.classList.contains('active')) {
if (!pantallaSubmenu.contains(e.target) && !gpioSubmenu.contains(e.target) && !sidebar.contains(e.target)) {
closeAllSubmenus();
}
}
});

navLinks.forEach(link => {
link.onclick = e => {
e.preventDefault();
const sectionId = link.dataset.section;

if (sectionId === 'salir') {
if (confirm('¿Estás seguro de que deseas salir? La placa se reiniciará.')) {
fetch('/exit').then(() => fetch('/restart')).then(() => {
showToast('Reiniciando placa...', 'info');
setTimeout(() => location.reload(), 2000);
}).catch(() => showToast('Error al reiniciar', 'error'));
}
return;
}

document.querySelectorAll('main section').forEach(section => {
section.classList.remove('active-section');
section.style.display = 'none';
});

let targetSectionElement = null;
if (sectionId === 'inicio') targetSectionElement = $('inicio-section');
else if (sectionId === 'datos') targetSectionElement = $('datos-section');
else if (sectionId === 'funciones') {
pantallaSubmenu.classList.add('active');
gpioSubmenu.classList.remove('active');
submenuOverlay.style.display = 'block';
render('pantalla');
pantallaSubmenuLinks.forEach(l => l.classList.remove('active'));
const pantallaLink = pantallaSubmenu.querySelector('a[data-subsection="pantalla"]');
if (pantallaLink) pantallaLink.classList.add('active');
$('funciones-content').classList.add('active-section');
$('funciones-content').style.display = 'block';
} else if (sectionId === 'gpio') {
gpioSubmenu.classList.add('active');
pantallaSubmenu.classList.remove('active');
submenuOverlay.style.display = 'block';
render('gpio');
gpioSubmenuLinks.forEach(l => l.classList.remove('active'));
const gpioLink = gpioSubmenu.querySelector('a[data-subsection="gpio"]');
if (gpioLink) gpioLink.classList.add('active');
$('funciones-content').classList.add('active-section');
$('funciones-content').style.display = 'block';
}

if (targetSectionElement) {
targetSectionElement.classList.add('active-section');
targetSectionElement.style.display = 'block';
}

navLinks.forEach(nl => nl.classList.remove('active'));
link.classList.add('active');

sidebar.classList.remove('active');
mainContent.classList.remove('shifted');
mainContent.style.marginLeft = '0';
menuToggle.classList.remove('active');
submenuOverlay.style.display = 'none';
};
});

pantallaSubmenuLinks.forEach(link => {
link.onclick = e => {
e.preventDefault();
pantallaSubmenuLinks.forEach(l => l.classList.remove('active'));
link.classList.add('active');
render(link.dataset.subsection);
};
});

gpioSubmenuLinks.forEach(link => {
link.onclick = e => {
e.preventDefault();
gpioSubmenuLinks.forEach(l => l.classList.remove('active'));
link.classList.add('active');
render(link.dataset.subsection);
};
});

const idInput = $('idDispositivo');
const canalInput = $('canalLoRa');
const guardarBtn = $('guardarDatos');
const resetBtn = $('restablecerDatos');
const status = $('datosStatus');

idInput.value = sistema.datos.id || '';
canalInput.value = sistema.datos.canal || '';

idInput.addEventListener('input', function() {
this.value = this.value.replace(/[^a-zA-Z0-9_]/g, '').replace(/ñ/gi, '');
});

canalInput.addEventListener('input', function() {
this.value = this.value.replace(/[^0-8]/g, '');
if (this.value > 8) this.value = 8;
});

if (guardarBtn) {
guardarBtn.onclick = () => {
const id = idInput.value.trim();
const canal = canalInput.value.trim();
const idValido = /^[a-zA-Z0-9_]+$/.test(id) && !/[ñÑ]/.test(id);
const canalValido = /^[0-8]$/.test(canal);

if (id && canal && idValido && canalValido) {
sistema.datos.id = id;
sistema.datos.canal = canal;
status.textContent = 'Datos guardados exitosamente.';
status.style.color = '#2ecc71';
showToast('Datos guardados exitosamente!', 'success');
guardarEstado();
actualizarResumenInicio();
} else {
showToast(!idValido ? 'Error: El ID contiene caracteres no permitidos (ñ o especiales).' :
!canalValido ? 'Error: El Canal debe ser un número entre 0 y 8.' :
'Error: ID y Canal son obligatorios.', 'error');
status.textContent = 'Error en los datos ingresados.';
status.style.color = '#e74c3c';
}
};
}

if (resetBtn) {
resetBtn.onclick = () => {
idInput.value = '';
canalInput.value = '';
sistema.datos.id = '';
sistema.datos.canal = '';
status.textContent = '';
showToast('Datos restablecidos.', 'info');
guardarEstado();
actualizarResumenInicio();
};
}
}

// Inicialización
cargarEstado();
setupNavigationYDatos();
render('inicio');
const initialSectionLink = document.querySelector('#perro_siete ul li a[data-section="inicio"]');
if (initialSectionLink) initialSectionLink.click();
});