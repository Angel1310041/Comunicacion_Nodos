/**
 * Script.js - Versión adaptada para GPIO solo con Archivo
 */
document.addEventListener('DOMContentLoaded', () => {
    const $ = id => document.getElementById(id);

    // Estado global
    let sistema = {
        pantalla: null,
        wifi: { ssid: '', password: '', connected: false, availableNetworks: [], savedNetworks: [] },
        datos: { id: '', canal: '' }
    };
    let currentSsidToConnect = '';

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
        localStorage.setItem('datos', JSON.stringify(sistema.datos));
        localStorage.setItem('pantalla', JSON.stringify(sistema.pantalla));
        localStorage.setItem('wifi', JSON.stringify({ ssid: sistema.wifi.ssid, connected: sistema.wifi.connected }));
    }

    function cargarEstado() {
        try {
            const datos = JSON.parse(localStorage.getItem('datos'));
            if (datos) sistema.datos = datos;
            const p = JSON.parse(localStorage.getItem('pantalla'));
            if (typeof p === 'boolean' || p === null) sistema.pantalla = p;
            const w = JSON.parse(localStorage.getItem('wifi'));
            if (w) {
                sistema.wifi.ssid = w.ssid || '';
                sistema.wifi.connected = !!w.connected;
            }
        } catch (e) {}
    }

    // 2. Modal WiFi
    function showPasswordModal(ssid, isUpdate = false) {
        currentSsidToConnect = ssid;
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
        guardarRed(currentSsidToConnect, password);
        hidePasswordModal();
    };

    $('cancelWifiButton').onclick = hidePasswordModal;

    // 3. Render y lógica principal
    function render(seccion) {
        // Actualiza resumen
        $('perro_diecinueve').textContent = sistema.pantalla === null ? '----' : (sistema.pantalla ? 'ENCENDIDA' : 'APAGADA');
        $('perro_veintiuno').textContent = sistema.wifi.connected ? `Conectado a: ${sistema.wifi.ssid}` : '----';
        $('perro_veintiuno').style.color = sistema.wifi.connected ? '#2ecc71' : '#e74c3c';
        $('perro_id_value').textContent = sistema.datos.id || '---';
        $('perro_canal_value').textContent = sistema.datos.canal || '---';

        // Renderiza subsección
        if (seccion === 'pantalla') {
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
                    sistema.pantalla = true; guardarEstado(); render('pantalla'); showToast('Pantalla encendida', 'success');
                }).catch(() => showToast('Error al encender pantalla', 'error'));
            };

            $('pantallaSubApagar').onclick = () => {
                fetch('/pantalla/off').then(res => res.json()).then(() => {
                    sistema.pantalla = false; guardarEstado(); render('pantalla'); showToast('Pantalla apagada', 'success');
                }).catch(() => showToast('Error al apagar pantalla', 'error'));
            };

        } else if (seccion === 'wifi') {
            $('funciones-content').innerHTML = `
                <div class="wifi-sub-section">
                    <h4>Configuración de WiFi</h4>
                    <div class="wifi-status-row">
                        <span>Estado actual: <strong id="wifiSubEstado">${sistema.wifi.connected ? `Conectado a: ${sistema.wifi.ssid}` : 'No conectado'}</strong></span>
                    </div>
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

        } else if (seccion === 'gpio') {
            // Mostrar barra lateral GPIO
            $('gpioSubmenu').classList.add('active');
            // Render inicial (mostrará archivo por defecto)
            renderGpioSubsection('archivo');
        }
    }

    // Renderizar subsecciones de GPIO
    function renderGpioSubsection(subsection) {
        if (subsection === 'archivo') {
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

            // Configurar eventos para los botones
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
        } else {
            $('funciones-content').innerHTML = '';
        }
    }

    // Funciones para manejo de archivos GPIO
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
            headers: {
                'Content-Type': 'application/json',
            },
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

    // 4. Lógica WiFi
    function setupWifiSection() {
        // ... (código existente de setupWifiSection) ...
    }

    // 5. Navegación y datos
    function setupNavigationYDatos() {
        const sidebar = $('perro_cinco'), menuToggle = $('perro_dos'), mainContent = $('perro_doce');
        const navLinks = document.querySelectorAll('#perro_siete ul li a');
        const pantallaSubmenu = $('pantallaSubmenu'), pantallaSubmenuLinks = pantallaSubmenu.querySelectorAll('li a[data-subsection]');
        const gpioSubmenu = $('gpioSubmenu'), gpioSubmenuLinks = gpioSubmenu.querySelectorAll('li a[data-gpiosubsection]');
        const FUNCIONES_SIDEBAR_WIDTH = 150;
        const GPIO_SUBMENU_WIDTH = 120;

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

        // --- NUEVO: Cerrar submenús al hacer clic en el área principal ---
        mainContent.addEventListener('click', function (e) {
            // Si algún submenú está abierto y el click NO fue dentro de los submenús ni del sidebar
            if (
                pantallaSubmenu.classList.contains('active') ||
                gpioSubmenu.classList.contains('active')
            ) {
                if (
                    pantallaSubmenu.contains(e.target) ||
                    gpioSubmenu.contains(e.target) ||
                    sidebar.contains(e.target)
                ) {
                    return; // No cerrar si el click fue dentro de los menús
                }
                closeAllSubmenus();
            }
        });
        // --- FIN NUEVO ---

        navLinks.forEach(link => link.onclick = e => {
            e.preventDefault();
            const sectionId = link.dataset.section;
            if (sectionId !== 'funciones') {
                pantallaSubmenu.classList.remove('active');
                gpioSubmenu.classList.remove('active');
                submenuOverlay.style.display = 'none';
            }

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
        });

        pantallaSubmenuLinks.forEach(link => link.onclick = e => {
            e.preventDefault();
            pantallaSubmenuLinks.forEach(l => l.classList.remove('active'));
            link.classList.add('active');
            render(link.dataset.subsection);
        });

        // Eventos para la barra lateral GPIO
        gpioSubmenuLinks.forEach(link => link.onclick = e => {
            e.preventDefault();
            gpioSubmenuLinks.forEach(l => l.classList.remove('active'));
            link.classList.add('active');
            renderGpioSubsection(link.dataset.gpiosubsection);
        });

        // Formulario de datos
        const idInput = $('idDispositivo'), canalInput = $('canalLoRa'),
            guardarBtn = $('guardarDatos'), resetBtn = $('restablecerDatos'),
            status = $('datosStatus');

        idInput.value = sistema.datos.id || '';
        canalInput.value = sistema.datos.canal || '';

        idInput.addEventListener('input', function() {
            this.value = this.value.replace(/[^a-zA-Z0-9_]/g, '').replace(/ñ/gi, '');
        });

        canalInput.addEventListener('input', function() {
            this.value = this.value.replace(/[^0-8]/g, '');
            if (this.value > 8) this.value = 8;
        });

        if (guardarBtn) guardarBtn.onclick = () => {
            const id = idInput.value.trim(), canal = canalInput.value.trim();
            const idValido = /^[a-zA-Z0-9_]+$/.test(id) && !/[ñÑ]/.test(id);
            const canalValido = /^[0-8]$/.test(canal);

            if (id && canal && idValido && canalValido) {
                sistema.datos.id = id;
                sistema.datos.canal = canal;
                status.textContent = 'Datos guardados exitosamente.';
                status.style.color = '#2ecc71';
                showToast('Datos guardados exitosamente!', 'success');
                guardarEstado();
            } else {
                showToast(!idValido ? 'Error: El ID contiene caracteres no permitidos (ñ o especiales).' :
                    !canalValido ? 'Error: El Canal debe ser un número entre 0 y 8.' :
                    'Error: ID y Canal son obligatorios.', 'error');
                status.textContent = 'Error en los datos ingresados.';
                status.style.color = '#e74c3c';
            }
        };

        if (resetBtn) resetBtn.onclick = () => {
            idInput.value = '';
            canalInput.value = '';
            sistema.datos.id = '';
            sistema.datos.canal = '';
            status.textContent = '';
            showToast('Datos restablecidos.', 'info');
            guardarEstado();
        };
    }

    // Inicialización
    cargarEstado();
    setupNavigationYDatos();
    render('inicio');
    const initialSectionLink = document.querySelector('#perro_siete ul li a[data-section="inicio"]');
    if (initialSectionLink) initialSectionLink.click();
});