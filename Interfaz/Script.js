/**
 * Script.js - Versión completa con UART e I2C en menú lateral
 */
document.addEventListener('DOMContentLoaded', () => {
    const $ = id => document.getElementById(id);

    // Estado global
    let sistema = {
        pantalla: null,
        wifi: { ssid: '', password: '', connected: false, availableNetworks: [], savedNetworks: [] },
        datos: { id: '', canal: '' },
        uart: { baudrate: 115200, dataBits: 8, parity: 'none', stopBits: 1, enabled: false },
        i2c: { clockSpeed: 100, sdaPin: 21, sclPin: 22, enabled: false }
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
                    <h3><i class="fas fa-microchip"></i> Configuración UART</h3>
                    <div class="uart-config">
                        <div class="uart-status-control">
                            <span>Estado UART: <strong id="uartStatusText">${sistema.uart.enabled ? 'ACTIVADO' : 'DESACTIVADO'}</strong></span>
                            <div class="uart-toggle-buttons">
                                <button id="enableUartBtn" class="action-button ${sistema.uart.enabled ? 'active-button' : 'success-button'}">Activar UART</button>
                                <button id="disableUartBtn" class="action-button ${!sistema.uart.enabled ? 'active-button' : 'danger-button'}">Desactivar UART</button>
                            </div>
                        </div>
                        
                        <div class="form-group">
                            <label>Baudrate:</label>
                            <select id="uartBaudrate" class="form-control" ${!sistema.uart.enabled ? 'disabled' : ''}>
                                ${[9600, 19200, 38400, 57600, 115200].map(b => 
                                    `<option value="${b}" ${sistema.uart.baudrate === b ? 'selected' : ''}>${b}</option>`
                                ).join('')}
                            </select>
                        </div>
                        
                        <div class="form-group">
                            <label>Bits de datos:</label>
                            <select id="uartDataBits" class="form-control" ${!sistema.uart.enabled ? 'disabled' : ''}>
                                ${[5, 6, 7, 8].map(b => 
                                    `<option value="${b}" ${sistema.uart.dataBits === b ? 'selected' : ''}>${b}</option>`
                                ).join('')}
                            </select>
                        </div>
                        
                        <div class="form-group">
                            <label>Paridad:</label>
                            <select id="uartParity" class="form-control" ${!sistema.uart.enabled ? 'disabled' : ''}>
                                <option value="none" ${sistema.uart.parity === 'none' ? 'selected' : ''}>Ninguna</option>
                                <option value="even" ${sistema.uart.parity === 'even' ? 'selected' : ''}>Par</option>
                                <option value="odd" ${sistema.uart.parity === 'odd' ? 'selected' : ''}>Impar</option>
                            </select>
                        </div>
                        
                        <div class="form-group">
                            <label>Bits de parada:</label>
                            <select id="uartStopBits" class="form-control" ${!sistema.uart.enabled ? 'disabled' : ''}>
                                <option value="1" ${sistema.uart.stopBits === 1 ? 'selected' : ''}>1</option>
                                <option value="2" ${sistema.uart.stopBits === 2 ? 'selected' : ''}>2</option>
                            </select>
                        </div>
                        
                        <button id="guardarUart" class="btn btn-primary" ${!sistema.uart.enabled ? 'disabled' : ''}>
                            <i class="fas fa-save"></i> Guardar Configuración
                        </button>
                        
                        <div class="terminal-container">
                            <h4>Terminal UART</h4>
                            <div id="uartTerminal" class="terminal"></div>
                            <div class="terminal-input">
                                <input type="text" id="uartCommand" placeholder="Escribe un comando..." class="form-control" ${!sistema.uart.enabled ? 'disabled' : ''}>
                                <button id="enviarUart" class="btn btn-secondary" ${!sistema.uart.enabled ? 'disabled' : ''}>
                                    <i class="fas fa-paper-plane"></i> Enviar
                                </button>
                            </div>
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
                        $('uartStatusText').textContent = 'ACTIVADO';
                        $('enableUartBtn').classList.add('active-button');
                        $('enableUartBtn').classList.remove('success-button');
                        $('disableUartBtn').classList.remove('active-button');
                        $('disableUartBtn').classList.add('danger-button');
                        
                        // Habilitar controles
                        $('uartBaudrate').disabled = false;
                        $('uartDataBits').disabled = false;
                        $('uartParity').disabled = false;
                        $('uartStopBits').disabled = false;
                        $('guardarUart').disabled = false;
                        $('uartCommand').disabled = false;
                        $('enviarUart').disabled = false;
                        
                        showToast('UART activado correctamente', 'success');
                        guardarEstado();
                    } else {
                        showToast('Error al activar UART', 'error');
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
                        $('uartStatusText').textContent = 'DESACTIVADO';
                        $('disableUartBtn').classList.add('active-button');
                        $('disableUartBtn').classList.remove('danger-button');
                        $('enableUartBtn').classList.remove('active-button');
                        $('enableUartBtn').classList.add('success-button');
                        
                        // Deshabilitar controles
                        $('uartBaudrate').disabled = true;
                        $('uartDataBits').disabled = true;
                        $('uartParity').disabled = true;
                        $('uartStopBits').disabled = true;
                        $('guardarUart').disabled = true;
                        $('uartCommand').disabled = true;
                        $('enviarUart').disabled = true;
                        
                        showToast('UART desactivado correctamente', 'success');
                        guardarEstado();
                    } else {
                        showToast('Error al desactivar UART', 'error');
                    }
                })
                .catch(() => showToast('Error de conexión', 'error'));
            };

            $('guardarUart').onclick = () => {
                sistema.uart = {
                    baudrate: parseInt($('uartBaudrate').value),
                    dataBits: parseInt($('uartDataBits').value),
                    parity: $('uartParity').value,
                    stopBits: parseInt($('uartStopBits').value),
                    enabled: sistema.uart.enabled
                };
                
                fetch('/setUartConfig', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify(sistema.uart)
                })
                .then(res => res.json())
                .then(data => {
                    if (data.success) {
                        showToast('Configuración UART guardada', 'success');
                        guardarEstado();
                    } else {
                        showToast('Error al guardar configuración UART', 'error');
                    }
                })
                .catch(() => showToast('Error de conexión', 'error'));
            };

            $('enviarUart').onclick = () => {
                const command = $('uartCommand').value.trim();
                if (!command) return;
                
                fetch('/sendUartCommand', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({command})
                })
                .then(res => res.json())
                .then(data => {
                    if (data.success) {
                        const terminal = $('uartTerminal');
                        terminal.innerHTML += `> ${command}\n< ${data.response || "Sin respuesta"}\n`;
                        terminal.scrollTop = terminal.scrollHeight;
                        $('uartCommand').value = '';
                    } else {
                        showToast('Error al enviar comando', 'error');
                    }
                })
                .catch(() => showToast('Error de conexión', 'error'));
            };

            // Permitir enviar con Enter
            $('uartCommand').addEventListener('keypress', (e) => {
                if (e.key === 'Enter') {
                    $('enviarUart').click();
                }
            });

        } else if (subsection === 'i2c') {
            $('funciones-content').innerHTML = `
                <div class="config-section">
                    <h3><i class="fas fa-microchip"></i> Configuración I2C</h3>
                    <div class="i2c-config">
                        <div class="i2c-status-control">
                            <span>Estado I2C: <strong id="i2cStatusText">${sistema.i2c.enabled ? 'ACTIVADO' : 'DESACTIVADO'}</strong></span>
                            <div class="i2c-toggle-buttons">
                                <button id="enableI2cBtn" class="action-button ${sistema.i2c.enabled ? 'active-button' : 'success-button'}">Activar I2C</button>
                                <button id="disableI2cBtn" class="action-button ${!sistema.i2c.enabled ? 'active-button' : 'danger-button'}">Desactivar I2C</button>
                                <button id="testI2cBtn" class="action-button info-button" ${!sistema.i2c.enabled ? 'disabled' : ''}>
                                    <i class="fas fa-vial"></i> Pruebas I2C
                                </button>
                            </div>
                        </div>
                        
                        <div class="form-group">
                            <label>Velocidad (kHz):</label>
                            <select id="i2cSpeed" class="form-control" ${!sistema.i2c.enabled ? 'disabled' : ''}>
                                ${[100, 400, 1000].map(s => 
                                    `<option value="${s}" ${sistema.i2c.clockSpeed === s ? 'selected' : ''}>${s}</option>`
                                ).join('')}
                            </select>
                        </div>
                        
                        <div class="form-group">
                            <label>Pin SDA:</label>
                            <select id="i2cSda" class="form-control" ${!sistema.i2c.enabled ? 'disabled' : ''}>
                                ${[21, 22, 33, 34].map(p => 
                                    `<option value="${p}" ${sistema.i2c.sdaPin === p ? 'selected' : ''}>${p}</option>`
                                ).join('')}
                            </select>
                        </div>
                        
                        <div class="form-group">
                            <label>Pin SCL:</label>
                            <select id="i2cScl" class="form-control" ${!sistema.i2c.enabled ? 'disabled' : ''}>
                                ${[22, 21, 33, 34].map(p => 
                                    `<option value="${p}" ${sistema.i2c.sclPin === p ? 'selected' : ''}>${p}</option>`
                                ).join('')}
                            </select>
                        </div>
                        
                        <button id="guardarI2c" class="btn btn-primary" ${!sistema.i2c.enabled ? 'disabled' : ''}>
                            <i class="fas fa-save"></i> Guardar Configuración
                        </button>
                        
                        <div class="i2c-devices">
                            <h4>Dispositivos I2C</h4>
                            <button id="scanI2c" class="btn btn-info" ${!sistema.i2c.enabled ? 'disabled' : ''}>
                                <i class="fas fa-search"></i> Escanear Dispositivos
                            </button>
                            <div id="i2cList" class="device-list"></div>
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
                        $('i2cStatusText').textContent = 'ACTIVADO';
                        $('enableI2cBtn').classList.add('active-button');
                        $('enableI2cBtn').classList.remove('success-button');
                        $('disableI2cBtn').classList.remove('active-button');
                        $('disableI2cBtn').classList.add('danger-button');
                        $('testI2cBtn').disabled = false;
                        $('i2cSpeed').disabled = false;
                        $('i2cSda').disabled = false;
                        $('i2cScl').disabled = false;
                        $('guardarI2c').disabled = false;
                        $('scanI2c').disabled = false;
                        showToast('I2C activado correctamente', 'success');
                        guardarEstado();
                    } else {
                        showToast('Error al activar I2C', 'error');
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
                        $('i2cStatusText').textContent = 'DESACTIVADO';
                        $('disableI2cBtn').classList.add('active-button');
                        $('disableI2cBtn').classList.remove('danger-button');
                        $('enableI2cBtn').classList.remove('active-button');
                        $('enableI2cBtn').classList.add('success-button');
                        $('testI2cBtn').disabled = true;
                        $('i2cSpeed').disabled = true;
                        $('i2cSda').disabled = true;
                        $('i2cScl').disabled = true;
                        $('guardarI2c').disabled = true;
                        $('scanI2c').disabled = true;
                        showToast('I2C desactivado correctamente', 'success');
                        guardarEstado();
                    } else {
                        showToast('Error al desactivar I2C', 'error');
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

            $('guardarI2c').onclick = () => {
                sistema.i2c = {
                    clockSpeed: parseInt($('i2cSpeed').value),
                    sdaPin: parseInt($('i2cSda').value),
                    sclPin: parseInt($('i2cScl').value),
                    enabled: sistema.i2c.enabled
                };
                
                fetch('/setI2cConfig', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify(sistema.i2c)
                })
                .then(res => res.json())
                .then(data => {
                    if (data.success) {
                        showToast('Configuración I2C guardada', 'success');
                        guardarEstado();
                    } else {
                        showToast('Error al guardar configuración I2C', 'error');
                    }
                })
                .catch(() => showToast('Error de conexión', 'error'));
            };

            $('scanI2c').onclick = () => {
                const list = $('i2cList');
                list.innerHTML = '<p class="loading-message">Escaneando dispositivos I2C...</p>';
                
                fetch('/scanI2c')
                .then(res => res.json())
                .then(data => {
                    if (data.devices && data.devices.length > 0) {
                        list.innerHTML = data.devices.map(dev => `
                            <div class="device">
                                <span class="address">0x${dev.address.toString(16).toUpperCase()}</span>
                                <span class="name">${dev.name || 'Dispositivo desconocido'}</span>
                                ${dev.registers ? `<span class="registers">${dev.registers} registros</span>` : ''}
                            </div>
                        `).join('');
                    } else {
                        list.innerHTML = '<p class="no-data-message">No se encontraron dispositivos I2C</p>';
                    }
                })
                .catch(() => {
                    list.innerHTML = '<p class="no-data-message">Error al escanear dispositivos</p>';
                    showToast('Error al escanear I2C', 'error');
                });
            };

        } else if (subsection === 'gpio') {
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
    function guardarRedConValidacion(ssid, password) {
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
                setTimeout(() => {
                    fetch('/restart').catch(() => {});
                }, 2000);
            } else {
                showToast('Error al guardar la red', 'error');
            }
        })
        .catch(() => showToast('Error al guardar la red', 'error'));
    }

    function actualizarRedConValidacion(ssid, password) {
        showToast('Actualizando red...', 'info');
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
            setTimeout(() => {
                fetch('/restart').catch(() => {});
            }, 2000);
        })
        .catch(() => showToast('Error al conectar a la red', 'error'));
    }

    function obtenerRedesGuardadas() {
        const savedTableBody = document.getElementById('savedNetworksTableBody');
        const noSavedMsg = document.getElementById('noSavedNetworks');
        if (savedTableBody) {
            savedTableBody.innerHTML = `<tr><td colspan="2" class="no-data-message">Cargando redes guardadas...</td></tr>`;
        }
        
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
        const pantallaSubmenuLinks = pantallaSubmenu.querySelectorAll('li a[data-subsection]');
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
            if (pantallaSubmenu.classList.contains('active')) {
                if (!pantallaSubmenu.contains(e.target) && !sidebar.contains(e.target)) {
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
                    submenuOverlay.style.display = 'block';
                    render('pantalla');
                    pantallaSubmenuLinks.forEach(l => l.classList.remove('active'));
                    const pantallaLink = pantallaSubmenu.querySelector('a[data-subsection="pantalla"]');
                    if (pantallaLink) pantallaLink.classList.add('active');
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