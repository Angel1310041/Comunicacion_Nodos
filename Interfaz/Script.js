document.addEventListener('DOMContentLoaded', function() {
    // Variables globales
    const apiUrl = 'http://' + window.location.hostname + '/api';
    let currentConfig = {};
document.getElementById('lora-id').addEventListener('input', function () {
    // Eliminar caracteres que no sean letras o números y limitar a 3 caracteres
    this.value = this.value
        .replace(/[^A-Za-z0-9]/g, '')  // Solo letras y números
        .replace(/ñ/gi, '')            // Elimina la ñ (mayúscula o minúscula)
        .substring(0, 3);              // Limita a 3 caracteres
});
document.getElementById('lora-channel').addEventListener('input', function () {
    const valor = this.value.replace(/[^0-9]/g, ''); // Solo dígitos
    if (valor === '' || parseInt(valor) < 0 || parseInt(valor) > 8) {
        this.value = ''; // Limpiar si no está en el rango
    } else {
        this.value = valor[0]; // Asegura que sea un solo número
    }
});

    // Elementos del DOM
    const menuBtn = document.getElementById('menu-btn');
    const sidebar = document.getElementById('sidebar');
    const mainContent = document.getElementById('main-content');
    const menuLinks = document.querySelectorAll('.menu-list a');
    const sections = document.querySelectorAll('.content-section');
    const i2cDevicesList = document.getElementById('i2c-devices');
    const wifiEnabledCheckbox = document.getElementById('wifi-enabled');

    // Inicialización
    initMenu();
    loadCurrentConfig();
    setupEventListeners();
    initWiFiSection();

    function initMenu() {
        // Botón de menú hamburguesa
        menuBtn.addEventListener('click', function() {
            sidebar.classList.toggle('active');
            mainContent.classList.toggle('shifted');
            toggleMenuAnimation();
        });

        // Submenú desplegable
        const submenuToggle = document.querySelector('.submenu-toggle');
        if (submenuToggle) {
            submenuToggle.addEventListener('click', function(e) {
                e.preventDefault();
                this.parentElement.classList.toggle('active');
                document.querySelector('.submenu').classList.toggle('active');
            });
        }

        // Navegación entre secciones
        menuLinks.forEach(link => {
            link.addEventListener('click', function(e) {
                if (!this.classList.contains('submenu-toggle')) {
                    e.preventDefault();
                    showSection(this.getAttribute('data-section'));
                    closeMenuOnMobile();
                }
            });
        });
    }

    function toggleMenuAnimation() {
        const spans = menuBtn.querySelectorAll('span');
        if (sidebar.classList.contains('active')) {
            spans[0].style.transform = 'rotate(45deg) translate(5px, 5px)';
            spans[1].style.opacity = '0';
            spans[2].style.transform = 'rotate(-45deg) translate(7px, -6px)';
        } else {
            spans[0].style.transform = 'none';
            spans[1].style.opacity = '1';
            spans[2].style.transform = 'none';
        }
    }

    function closeMenuOnMobile() {
        if (window.innerWidth <= 768) {
            sidebar.classList.remove('active');
            mainContent.classList.remove('shifted');
            toggleMenuAnimation();
        }
    }

    function showSection(sectionId) {
        sections.forEach(section => section.classList.remove('active'));
        const targetSection = sectionId === 'salir' ?
            document.getElementById('salir-section') :
            document.getElementById(`${sectionId}-section`);
        if (targetSection) targetSection.classList.add('active');
    }

    async function loadCurrentConfig() {
        try {
            const response = await fetch(`${apiUrl}/config`);
            currentConfig = await response.json();

            document.getElementById('current-id').textContent = currentConfig.id || 'HELTEC01';
            document.getElementById('current-channel').textContent = currentConfig.channel || '0';
            document.getElementById('current-wifi').textContent = currentConfig.WiFi ? 'Activado' : 'Desactivado';
            document.getElementById('lora-id').value = currentConfig.id || 'HELTEC01';
            document.getElementById('lora-channel').value = currentConfig.channel || '0';
            wifiEnabledCheckbox.checked = currentConfig.WiFi || false;

            updateButtonStates();
        } catch (error) {
            console.error('Error al cargar configuración:', error);
            setDefaultValues();
        }
    }

    function setDefaultValues() {
        document.getElementById('current-id').textContent = 'HELTEC01';
        document.getElementById('current-channel').textContent = '0';
        document.getElementById('current-wifi').textContent = 'Desactivado';
        document.getElementById('lora-id').value = 'HELTEC01';
        document.getElementById('lora-channel').value = '0';
        wifiEnabledCheckbox.checked = false;
    }

    function updateButtonStates() {
        const updateButton = (elementId, isActive) => {
            const btn = document.getElementById(elementId);
            if (btn) {
                btn.classList.toggle('active', isActive);
            }
        };

        updateButton('activar-pantalla', currentConfig.displayOn);
        updateButton('apagar-pantalla', !currentConfig.displayOn);
        updateButton('activar-uart', currentConfig.UART);
        updateButton('apagar-uart', !currentConfig.UART);
        updateButton('activar-wifi', currentConfig.WiFi);
        updateButton('apagar-wifi', !currentConfig.WiFi);
        updateButton('escaner-i2c', currentConfig.I2C);
        updateButton('desactivar-i2c', !currentConfig.I2C);
    }

    function setupEventListeners() {
        // Formulario de configuración LoRa
        const configForm = document.getElementById('config-form');
        if (configForm) {
            configForm.addEventListener('submit', async function(e) {
                e.preventDefault();
                const id = document.getElementById('lora-id').value;
                const channel = document.getElementById('lora-channel').value;
                const wifiEnabled = wifiEnabledCheckbox.checked;

                if (!validateConfigInput(id, channel)) return;

                try {
                    const response = await fetch(`${apiUrl}/config`, {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ id, channel, WiFi: wifiEnabled })
                    });
                    const data = await response.json();

                    if (data.success) {
                        showAlert('success', 'Configuración guardada correctamente');
                        updateUIWithNewConfig(id, channel, wifiEnabled);
                    } else {
                        throw new Error(data.message || 'Error desconocido');
                    }
                } catch (error) {
                    showError('Error al guardar configuración:', error);
                }
            });
        }

        // Asignar eventos a los botones
        setupButton('activar-pantalla', () => sendDisplayCommand('1'));
        setupButton('apagar-pantalla', () => sendDisplayCommand('0'));
        setupButton('prueba-pantalla', () => {
            const currentId = document.getElementById('current-id').textContent;
            sendDisplayTestCommand(currentId);
        });

        setupButton('activar-uart', () => sendUartCommand('1'));
        setupButton('apagar-uart', () => sendUartCommand('0'));

        setupButton('escaner-i2c', scanI2CDevices);
        setupButton('desactivar-i2c', () => sendI2CCommand('0'));

        // Configuración de los botones de salida
// Reemplaza esta parte del código:
setupButton('reiniciar-dispositivo', () => {
    showConfirmDialog('¿Estás seguro que deseas reiniciar la tarjeta Heltec?', async () => {
        try {
            const response = await fetch(`${apiUrl}/reboot`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' }
            });

            if (response.ok) {
                showRebootMessage();
            } else {
                throw new Error('Error al enviar comando de reinicio');
            }
        } catch (error) {
            showError('Error al reiniciar:', error);
        }
    });
});

setupButton('salir-programacion', () => {
    showConfirmDialog('¿Estás seguro que deseas salir del modo de programación? Esto reiniciará el dispositivo en modo normal.', async () => {
        try {
            const response = await fetch(`${apiUrl}/exit-programming`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' }
            });

            if (response.ok) {
                showExitProgrammingMessage();
            } else {
                throw new Error('Error al salir del modo programación');
            }
        } catch (error) {
            showError('Error al salir del modo programación:', error);
        }
    });
});

setupButton('cancelar-salir', () => showSection('inicio'));

// Con este nuevo código:
setupButton('salir-reiniciar', () => {
    showConfirmDialog('¿Estás seguro que deseas salir del modo de programación y reiniciar la tarjeta? Esto apagará el LED y reiniciará el dispositivo.', async () => {
        try {
            const response = await fetch(`${apiUrl}/exit-programming`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' }
            });

            if (response.ok) {
                showExitProgrammingMessage();
            } else {
                throw new Error('Error al salir del modo programación');
            }
        } catch (error) {
            showError('Error al salir del modo programación:', error);
        }
    });
});

        setupButton('cancelar-salir', () => showSection('inicio'));
    }

    function setupButton(id, handler) {
        const btn = document.getElementById(id);
        if (btn) btn.addEventListener('click', handler);
    }

    function validateConfigInput(id, channel) {
        if (!/^[A-Za-z0-9]{1,3}$/.test(id)) {
            showAlert('error', 'ID inválido. Solo letras y números, máximo 3 caracteres.');
            return false;
        }

        if (channel < 0 || channel > 8) {
            showAlert('error', 'Canal inválido. Debe ser entre 0 y 8.');
            return false;
        }

        return true;
    }

    function updateUIWithNewConfig(id, channel, wifiEnabled) {
        document.getElementById('current-id').textContent = id;
        document.getElementById('current-channel').textContent = channel;
        document.getElementById('current-wifi').textContent = wifiEnabled ? 'Activado' : 'Desactivado';
        currentConfig.id = id;
        currentConfig.channel = channel;
        currentConfig.WiFi = wifiEnabled;
    }

    function showRebootMessage() {
        const salirSection = document.getElementById('salir-section');
        if (salirSection) {
            salirSection.innerHTML = `
                <div class="funcion-container">
                    <h2>Reiniciando tarjeta Heltec...</h2>
                    <div class="reboot-message">
                        <p>La tarjeta se está reiniciando. Por favor, espera unos segundos.</p>
                        <div class="loading-spinner"></div>
                        <p>La página se recargará automáticamente.</p>
                    </div>
                </div>
            `;
        }

        // Intentar recargar después de 10 segundos
        setTimeout(() => {
            window.location.reload();
        }, 10000);
    }

    function showExitProgrammingMessage() {
        const salirSection = document.getElementById('salir-section');
        if (salirSection) {
            salirSection.innerHTML = `
                <div class="funcion-container">
                    <h2>Saliendo del modo programación...</h2>
                    <div class="reboot-message">
                        <p>El dispositivo se está reiniciando en modo normal.</p>
                        <div class="loading-spinner"></div>
                        <p>La conexión se cerrará automáticamente.</p>
                    </div>
                </div>
            `;
        }

        // Intentar recargar después de 10 segundos
        setTimeout(() => {
            window.location.href = 'http://' + window.location.hostname;
        }, 10000);
    }

    // WiFi Management Functions
    function initWiFiSection() {
        // Control WiFi
        setupButton('activar-wifi', () => sendWiFiCommand('1'));
        setupButton('apagar-wifi', () => sendWiFiCommand('0'));

        // Escaneo WiFi
        setupButton('scan-wifi', scanWiFiNetworks);
        setupButton('refresh-wifi', scanWiFiNetworks);

        // Redes guardadas
        setupButton('show-saved', showSavedNetworks);
        setupButton('remove-all-wifi', removeAllNetworks);
    }

    async function sendWiFiCommand(state) {
        try {
            const response = await fetch(`${apiUrl}/wifi`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ state })
            });
            const data = await response.json();

            if (data.success) {
                showAlert('success', `WiFi ${state === '1' ? 'activado' : 'desactivado'} correctamente`);
                currentConfig.WiFi = (state === '1');
                document.getElementById('current-wifi').textContent = currentConfig.WiFi ? 'Activado' : 'Desactivado';
                updateButtonStates();
            } else {
                throw new Error(data.message || 'Error desconocido');
            }
        } catch (error) {
            showError('Error al controlar WiFi:', error);
        }
    }

    function scanWiFiNetworks() {
        const wifiTable = document.getElementById('wifi-table');
        wifiTable.innerHTML = '<tr><td colspan="4" class="loading">Escaneando redes WiFi...</td></tr>';

        fetch(`${apiUrl}/wifi/scan`)
            .then(response => {
                if (!response.ok) throw new Error('Error en la respuesta del servidor');
                return response.json();
            })
            .then(data => {
                if (data.error) {
                    wifiTable.innerHTML = `<tr><td colspan="4" class="error">${data.error}</td></tr>`;
                } else if (data.redes && data.redes.length > 0) {
                    renderWiFiTable(data.redes);
                } else {
                    wifiTable.innerHTML = '<tr><td colspan="4">No se encontraron redes WiFi</td></tr>';
                }
            })
            .catch(error => {
                wifiTable.innerHTML = `<tr><td colspan="4" class="error">Error: ${error.message}</td></tr>`;
            });
    }

    function renderWiFiTable(networks) {
        const wifiTable = document.getElementById('wifi-table');
        wifiTable.innerHTML = `
            <tr>
                <th>SSID</th>
                <th>Seguridad</th>
                <th>Señal</th>
                <th>Acción</th>
            </tr>
        `;

        networks.forEach(network => {
            const row = document.createElement('tr');
            const strength = getStrengthLevel(network.rssi);
            const securityColor = getSecurityColor(network.encryptionType);

            row.innerHTML = `
                <td>${network.ssid || 'Oculta'}</td>
                <td style="color: ${securityColor}">${network.encryption}</td>
                <td>
                    <div class="signal-strength">
                        <div class="signal-bar ${strength > 0 ? 'active' : ''}"></div>
                        <div class="signal-bar ${strength > 1 ? 'active' : ''}"></div>
                        <div class="signal-bar ${strength > 2 ? 'active' : ''}"></div>
                        <div class="signal-bar ${strength > 3 ? 'active' : ''}"></div>
                    </div>
                </td>
                <td>
                    <button class="connect-btn" data-ssid="${network.ssid || ''}" data-encryption="${network.encryptionType}">
                        Conectar
                    </button>
                </td>
            `;
            wifiTable.appendChild(row);
        });

        // Agregar event listeners a los botones de conexión
        document.querySelectorAll('.connect-btn').forEach(btn => {
            btn.addEventListener('click', function() {
                const ssid = this.getAttribute('data-ssid');
                const encryption = this.getAttribute('data-encryption');
                showPasswordDialog(ssid, encryption);
            });
        });
    }

    function getStrengthLevel(rssi) {
        if (rssi >= -50) return 4; // Excelente
        if (rssi >= -60) return 3; // Bueno
        if (rssi >= -70) return 2; // Regular
        if (rssi >= -80) return 1; // Débil
        return 0; // Muy débil
    }

    function getSecurityColor(encryptionType) {
        if (encryptionType === 0) return '#2ecc71'; // Abierta (verde)
        if (encryptionType === 1) return '#f39c12'; // WEP (naranja)
        return '#e74c3c'; // WPA/WPA2 (rojo)
    }

    function showPasswordDialog(ssid, encryptionType) {
        const dialog = document.createElement('div');
        dialog.className = 'password-dialog';
        dialog.innerHTML = `
            <div class="dialog-content">
                <h3>Conectar a "${ssid}"</h3>
                ${encryptionType > 0 ? `
                    <input type="password" id="wifi-password" placeholder="Contraseña WiFi" required>
                ` : '<p>Red abierta (sin contraseña)</p>'}
                <div class="dialog-buttons">
                    <button id="cancel-wifi">Cancelar</button>
                    <button id="confirm-wifi">Conectar</button>
                </div>
            </div>
        `;
        document.body.appendChild(dialog);

        document.getElementById('cancel-wifi').addEventListener('click', () => {
            document.body.removeChild(dialog);
        });

        document.getElementById('confirm-wifi').addEventListener('click', () => {
            const password = encryptionType > 0 ?
                document.getElementById('wifi-password').value : '';

            if (encryptionType > 0 && !password) {
                showAlert('error', 'Por favor ingresa la contraseña');
                return;
            }

            connectToWiFi(ssid, password, encryptionType);
            document.body.removeChild(dialog);
        });
    }

    async function connectToWiFi(ssid, password, encryptionType) {
        try {
            showAlert('info', `Conectando a ${ssid}...`);

            const response = await fetch(`${apiUrl}/wifi/connect`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid, password, encryptionType })
            });

            const data = await response.json();

            if (data.success) {
                showAlert('success', `Conectado a ${ssid} correctamente`);
                // Actualizar la lista de redes guardadas
                showSavedNetworks();
            } else {
                throw new Error(data.message || 'Error al conectar');
            }
        } catch (error) {
            showError('Error al conectar a WiFi:', error);
        }
    }

    async function showSavedNetworks() {
        const savedNetworksContainer = document.getElementById('saved-networks');
        savedNetworksContainer.innerHTML = '<p class="loading">Cargando redes guardadas...</p>';

        try {
            const response = await fetch(`${apiUrl}/wifi/saved`);
            const data = await response.json();

            if (data.error) {
                savedNetworksContainer.innerHTML = `<p class="error">${data.error}</p>`;
            } else if (data.networks && data.networks.length > 0) {
                renderSavedNetworks(data.networks);
            } else {
                savedNetworksContainer.innerHTML = '<p>No hay redes guardadas</p>';
            }
        } catch (error) {
            savedNetworksContainer.innerHTML = `<p class="error">Error: ${error.message}</p>`;
        }
    }

    function renderSavedNetworks(networks) {
        const savedNetworksContainer = document.getElementById('saved-networks');
        const list = document.createElement('ul');
        list.className = 'saved-networks-list';

        networks.forEach(network => {
            const item = document.createElement('li');
            item.innerHTML = `
                <span>${network.ssid}</span>
                <button class="remove-wifi" data-ssid="${network.ssid}">Eliminar</button>
            `;
            list.appendChild(item);
        });

        savedNetworksContainer.innerHTML = '';
        savedNetworksContainer.appendChild(list);

        // Agregar event listeners a los botones de eliminar
        document.querySelectorAll('.remove-wifi').forEach(btn => {
            btn.addEventListener('click', function() {
                const ssid = this.getAttribute('data-ssid');
                removeNetwork(ssid);
            });
        });
    }

    async function removeNetwork(ssid) {
        showConfirmDialog(`¿Eliminar la red "${ssid}" de las guardadas?`, async () => {
            try {
                const response = await fetch(`${apiUrl}/wifi/remove`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ ssid })
                });

                const data = await response.json();

                if (data.success) {
                    showAlert('success', `Red "${ssid}" eliminada correctamente`);
                    showSavedNetworks();
                } else {
                    throw new Error(data.message || 'Error al eliminar');
                }
            } catch (error) {
                showError('Error al eliminar red:', error);
            }
        });
    }

    async function removeAllNetworks() {
        showConfirmDialog('¿Eliminar TODAS las redes guardadas?', async () => {
            try {
                const response = await fetch(`${apiUrl}/wifi/remove-all`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' }
                });

                const data = await response.json();

                if (data.success) {
                    showAlert('success', 'Todas las redes eliminadas correctamente');
                    showSavedNetworks();
                } else {
                    throw new Error(data.message || 'Error al eliminar');
                }
            } catch (error) {
                showError('Error al eliminar redes:', error);
            }
        });
    }

    // Funciones para otras secciones
    async function sendDisplayCommand(state) {
        try {
            const response = await fetch(`${apiUrl}/display`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ state })
            });

            const data = await response.json();

            if (data.success) {
                showAlert('success', `Pantalla ${state === '1' ? 'activada' : 'apagada'}`);
                currentConfig.displayOn = (state === '1');
                updateButtonStates();
            } else {
                throw new Error(data.message || 'Error desconocido');
            }
        } catch (error) {
            showError('Error al controlar pantalla:', error);
        }
    }

    async function sendDisplayTestCommand(id) {
        try {
            const response = await fetch(`${apiUrl}/display/test`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ id })
            });

            const data = await response.json();

            if (data.success) {
                showAlert('success', 'Prueba de pantalla iniciada');
            } else {
                throw new Error(data.message || 'Error desconocido');
            }
        } catch (error) {
            showError('Error al probar pantalla:', error);
        }
    }

    async function sendUartCommand(state) {
        try {
            const response = await fetch(`${apiUrl}/uart`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ state })
            });

            const data = await response.json();

            if (data.success) {
                showAlert('success', `UART ${state === '1' ? 'activado' : 'desactivado'}`);
                currentConfig.UART = (state === '1');
                updateButtonStates();
            } else {
                throw new Error(data.message || 'Error desconocido');
            }
        } catch (error) {
            showError('Error al controlar UART:', error);
        }
    }

    async function scanI2CDevices() {
        i2cDevicesList.innerHTML = '<li class="loading">Buscando dispositivos I2C...</li>';

        try {
            const response = await fetch(`${apiUrl}/i2c/scan`);
            const data = await response.json();

            if (data.error) {
                i2cDevicesList.innerHTML = `<li class="error">${data.error}</li>`;
            } else if (data.devices && data.devices.length > 0) {
                renderI2CDevices(data.devices);
            } else {
                i2cDevicesList.innerHTML = '<li class="no-devices">No se encontraron dispositivos I2C</li>';
            }

            currentConfig.I2C = true;
            updateButtonStates();
        } catch (error) {
            i2cDevicesList.innerHTML = `<li class="error">Error: ${error.message}</li>`;
        }
    }

    function renderI2CDevices(devices) {
        i2cDevicesList.innerHTML = '';

        devices.forEach(device => {
            const item = document.createElement('li');
            item.className = 'device-item';
            item.innerHTML = `
                <span class="device-address">0x${device.address.toString(16)}</span>
                <span class="device-type">${device.type || 'Desconocido'}</span>
            `;
            i2cDevicesList.appendChild(item);
        });
    }

    async function sendI2CCommand(state) {
        try {
            const response = await fetch(`${apiUrl}/i2c`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ state })
            });

            const data = await response.json();

            if (data.success) {
                showAlert('success', `I2C ${state === '1' ? 'activado' : 'desactivado'}`);
                currentConfig.I2C = (state === '1');
                updateButtonStates();

                if (state === '0') {
                    i2cDevicesList.innerHTML = '<li class="no-devices">I2C desactivado</li>';
                }
            } else {
                throw new Error(data.message || 'Error desconocido');
            }
        } catch (error) {
            showError('Error al controlar I2C:', error);
        }
    }

    // Funciones de utilidad
    function showAlert(type, message) {
        const alert = document.createElement('div');
        alert.className = `alert ${type}`;
        alert.textContent = message;
        document.body.appendChild(alert);

        setTimeout(() => {
            alert.style.opacity = '0';
            setTimeout(() => document.body.removeChild(alert), 300);
        }, 3000);
    }

    function showError(context, error) {
        console.error(context, error);
        showAlert('error', `${context} ${error.message}`);
    }

    function showConfirmDialog(message, confirmCallback) {
        const dialog = document.createElement('div');
        dialog.className = 'confirm-dialog';
        dialog.innerHTML = `
            <div class="dialog-content">
                <h3>Confirmar</h3>
                <p>${message}</p>
                <div class="dialog-buttons">
                    <button id="cancel-action">Cancelar</button>
                    <button id="confirm-action">Confirmar</button>
                </div>
            </div>
        `;
        document.body.appendChild(dialog);

        document.getElementById('cancel-action').addEventListener('click', () => {
            document.body.removeChild(dialog);
        });

        document.getElementById('confirm-action').addEventListener('click', () => {
            confirmCallback();
            document.body.removeChild(dialog);
        });
    }
});