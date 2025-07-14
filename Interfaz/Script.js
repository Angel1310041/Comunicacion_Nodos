document.addEventListener('DOMContentLoaded', () => {
    // Selectores
    const $ = id => document.getElementById(id);
    const sidebar = $('perro_cinco');
    const menuToggle = $('perro_dos');
    const mainContent = $('perro_doce');
    const navLinks = document.querySelectorAll('#perro_siete ul li a');

    // Elementos de estado en la sección de Inicio
    const screenStatusSummary = $('perro_diecinueve');
    const wifiStatus = $('perro_veintiuno');
    const datosIdValue = $('perro_id_value');
    const datosCanalValue = $('perro_canal_value');

    // Submenú lateral Funciones
    const funcionesMenuLink = $('perro_nueve');
    const pantallaSubmenu = $('pantallaSubmenu');
    const pantallaSubmenuLinks = pantallaSubmenu.querySelectorAll('li a[data-subsection]');

    // --- AJUSTE: Reducir ancho del submenú lateral de funciones ---
    const FUNCIONES_SIDEBAR_WIDTH = 150;

    // Ajustar el ancho del submenú lateral
    pantallaSubmenu.style.width = FUNCIONES_SIDEBAR_WIDTH + 'px';
    pantallaSubmenu.style.minWidth = FUNCIONES_SIDEBAR_WIDTH + 'px';
    pantallaSubmenu.style.maxWidth = FUNCIONES_SIDEBAR_WIDTH + 'px';

    // Overlay pegado a la barra lateral
    const submenuOverlay = document.createElement('div');
    submenuOverlay.id = 'submenuOverlay';
    submenuOverlay.className = 'submenu-overlay';
    submenuOverlay.style.position = 'fixed';
    submenuOverlay.style.top = '0';
    submenuOverlay.style.bottom = '0';
    submenuOverlay.style.background = 'rgba(0,0,0,0.15)';
    submenuOverlay.style.zIndex = '100';
    submenuOverlay.style.display = 'none';
    document.body.appendChild(submenuOverlay);

    // Actualizar la posición y el ancho del overlay
    submenuOverlay.style.left = FUNCIONES_SIDEBAR_WIDTH + 'px';
    submenuOverlay.style.width = `calc(100vw - ${FUNCIONES_SIDEBAR_WIDTH}px)`;

    // Contenedor principal donde se cargan las subsecciones de funciones
    const funcionesContentContainer = $('funciones-content');

    // Datos Section Elements
    const datosFormSection = $('datos-section');
    const idDispositivoInput = $('idDispositivo');
    const canalLoRaInput = $('canalLoRa');
    const guardarDatosButton = $('guardarDatos');
    const restablecerDatosButton = $('restablecerDatos');
    const datosStatus = $('datosStatus');

    // Toast Container
    const toastContainer = $('toastContainer');

    // Estado del sistema (datos simulados)
    let sistema = {
        pantalla: null,
        wifi: {
            ssid: '',
            password: '',
            connected: false,
            availableNetworks: [],
            savedNetworks: [] // Inicialmente vacío
        },
        datos: {
            id: '',
            canal: ''
        }
    };

    // --- Modal Elements ---
    const wifiPasswordModal = $('wifiPasswordModal');
    const modalSsidName = $('modalSsidName');
    const wifiPasswordInput = $('wifiPasswordInput');
    const connectWifiButton = $('connectWifiButton');
    const cancelWifiButton = $('cancelWifiButton');
    let currentSsidToConnect = '';

    // --- Funciones de Utilidad ---

    const showToast = (message, type = 'info') => {
        const toast = document.createElement('div');
        toast.classList.add('toast', type);
        toast.textContent = message;
        toastContainer.appendChild(toast);

        setTimeout(() => {
            toast.style.opacity = '0';
            toast.style.transform = 'translateY(-20px)';
            toast.addEventListener('transitionend', () => toast.remove());
        }, 3000);
    };

    const updateStatus = () => {
        if (sistema.pantalla === null) {
            screenStatusSummary.textContent = '----';
        } else {
            screenStatusSummary.textContent = sistema.pantalla ? 'ENCENDIDA' : 'APAGADA';
        }
        const pantallaSubEstado = document.getElementById('pantallaSubEstado');
        if (pantallaSubEstado) {
            pantallaSubEstado.textContent = sistema.pantalla === null ? '----' : (sistema.pantalla ? 'Encendida' : 'Apagada');
        }
        if (!sistema.wifi.connected || !sistema.wifi.ssid) {
            wifiStatus.textContent = '----';
            wifiStatus.style.color = '#e74c3c';
        } else {
            wifiStatus.textContent = `Conectado a: ${sistema.wifi.ssid}`;
            wifiStatus.style.color = '#2ecc71';
        }
        datosIdValue.textContent = sistema.datos.id ? sistema.datos.id : '---';
        datosCanalValue.textContent = sistema.datos.canal ? sistema.datos.canal : '---';
    };

    // Función para obtener el ícono de intensidad de señal
    const getSignalIcon = (intensity) => {
        if (intensity >= 4) return '<i class="fas fa-wifi signal-icon"></i>';
        if (intensity >= 3) return '<i class="fas fa-wifi signal-icon"></i>';
        if (intensity >= 2) return '<i class="fas fa-wifi signal-icon medium"></i>';
        if (intensity >= 1) return '<i class="fas fa-wifi signal-icon weak"></i>';
        return '<i class="fas fa-wifi signal-icon weak"></i>';
    };

    // Function to show the password modal
    const showPasswordModal = (ssid, isSavedNetwork = false, expectedPassword = '') => {
        currentSsidToConnect = ssid;
        modalSsidName.textContent = `Conectar a ${ssid}`;
        wifiPasswordInput.value = '';
        wifiPasswordModal.classList.add('active');
        wifiPasswordInput.focus();
        
        // Configurar datos para validación si es una red guardada
        if (isSavedNetwork) {
            wifiPasswordModal.dataset.expectedPassword = expectedPassword;
            wifiPasswordModal.dataset.isSavedNetwork = 'true';
        } else {
            wifiPasswordModal.removeAttribute('data-expected-password');
            wifiPasswordModal.dataset.isSavedNetwork = 'false';
        }
        // Asegurarse de limpiar cualquier mensaje de error previo
        const existingError = wifiPasswordModal.querySelector('.password-error');
        if (existingError) {
            existingError.remove();
        }
        wifiPasswordInput.style.borderColor = ''; // Restablecer el color del borde
    };

    // Function to hide the password modal
    const hidePasswordModal = () => {
        wifiPasswordModal.classList.remove('active');
        currentSsidToConnect = '';
        wifiPasswordModal.removeAttribute('data-expected-password');
        wifiPasswordModal.removeAttribute('data-is-saved-network');
        
        // Limpiar mensajes de error y estilos
        const errorElement = wifiPasswordModal.querySelector('.password-error');
        if (errorElement) {
            errorElement.remove();
        }
        wifiPasswordInput.style.borderColor = '';
        wifiPasswordInput.value = '';
    };

    // Event listener for connect button in modal
    connectWifiButton.onclick = () => {
        const enteredPassword = wifiPasswordInput.value.trim();
        const ssid = currentSsidToConnect;
        const isSavedNetwork = wifiPasswordModal.dataset.isSavedNetwork === 'true';
        const expectedPassword = wifiPasswordModal.dataset.expectedPassword || '';

        // Remove any existing error message
        const existingError = wifiPasswordModal.querySelector('.password-error');
        if (existingError) {
            existingError.remove();
        }
        wifiPasswordInput.style.borderColor = ''; // Reset border color

        // Check if the network requires a password and if the password is empty
        const network = sistema.wifi.availableNetworks.find(net => net.ssid === ssid);
        const requiresPassword = network && network.security !== 'Abierta';

        if (requiresPassword && !enteredPassword) {
            const errorElement = document.createElement('div');
            errorElement.className = 'password-error';
            errorElement.textContent = 'Por favor, ingrese la contraseña';
            errorElement.style.color = '#e74c3c';
            errorElement.style.marginTop = '10px';
            errorElement.style.fontSize = '0.9em';
            
            wifiPasswordInput.insertAdjacentElement('afterend', errorElement);
            wifiPasswordInput.style.borderColor = '#e74c3c';
            showToast('Por favor, ingrese la contraseña', 'error');
            return; // Stop execution if password is required but not provided
        }

        if (isSavedNetwork) {
    // Para redes guardadas, validar contraseña
    if (enteredPassword === expectedPassword) {
        showToast('Contraseña correcta, conectado a la red.', 'success');
        connectToWifi(ssid, enteredPassword, true);
    } else {
        const errorElement = document.createElement('div');
        errorElement.className = 'password-error';
        errorElement.textContent = 'Contraseña inválida';
        errorElement.style.color = '#e74c3c';
        errorElement.style.marginTop = '10px';
        errorElement.style.fontSize = '0.9em';
        wifiPasswordInput.insertAdjacentElement('afterend', errorElement);
        wifiPasswordInput.style.borderColor = '#e74c3c';
        showToast('Contraseña inválida', 'error');
    }
} else {
    // Para redes nuevas, simular contraseña correcta "12345678"
    const fakeCorrectPassword = '12345678';
    if (enteredPassword === fakeCorrectPassword) {
        showToast('Contraseña correcta, conectado a la red.', 'success');
        connectToWifi(ssid, enteredPassword, false);
    } else {
        const errorElement = document.createElement('div');
        errorElement.className = 'password-error';
        errorElement.textContent = 'Contraseña inválida';
        errorElement.style.color = '#e74c3c';
        errorElement.style.marginTop = '10px';
        errorElement.style.fontSize = '0.9em';
        wifiPasswordInput.insertAdjacentElement('afterend', errorElement);
        wifiPasswordInput.style.borderColor = '#e74c3c';
        showToast('Contraseña inválida', 'error');
    }
}
    };

    // Event listener for cancel button in modal
    cancelWifiButton.onclick = () => {
        hidePasswordModal();
        showToast('Conexión cancelada.', 'info');
    };

    // --- Renderización de Contenido Dinámico para Subsecciones de "Funciones" ---

    const loadSubsectionContent = (subsection) => {
        let contentHtml = '';
        switch (subsection) {
            case 'pantalla':
                contentHtml = `
                    <div class="pantalla-status-box">
                        <h4>Control de Pantalla</h4>
                        <div class="pantalla-status-row">
                            <span>Estado actual: <strong id="pantallaSubEstado">----</strong></span>
                        </div>
                        <div class="pantalla-status-actions">
                            <button id="pantallaSubEncender" class="action-button success-button">
                                <i class="fas fa-power-off"></i> Encender
                            </button>
                            <button id="pantallaSubApagar" class="action-button danger-button">
                                <i class="fas fa-power-off"></i> Apagar
                            </button>
                        </div>
                    </div>
                `;
                break;
            case 'wifi':
                contentHtml = `
                    <div class="wifi-sub-section">
                        <h4>Configuración de WiFi</h4>
                        <div class="wifi-status-row">
                            <span>Estado actual: <strong id="wifiSubEstado">No conectado</strong></span>
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
                                        <tr><td colspan="4" class="no-data-message">Presione el botón para Obtener las Redes Disponibles</td></tr>
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
                                        <tr><td colspan="2" class="no-data-message">Presione el botón para Obtener las Redes Guardadas</td></tr>
                                    </tbody>
                                </table>
                            </div>
                            <p id="noSavedNetworks" class="no-data-message" style="display:none;">No hay redes guardadas.</p>
                        </div>
                    </div>
                `;
                break;
            case 'uart':
            case 'i2c':
            case 'gpio':
                contentHtml = '<p>Funcionalidad en desarrollo para esta sección.</p>';
                break;
            default:
                contentHtml = '<p>Seleccione una función del menú lateral.</p>';
        }
        funcionesContentContainer.innerHTML = contentHtml;
    };

    function showSubsection(sub) {
        funcionesContentContainer.classList.add('active-section');
        funcionesContentContainer.style.display = 'block';
        loadSubsectionContent(sub);

        if (sub === 'pantalla') {
            conectarBotonesPantallaSubmenu();
        } else if (sub === 'wifi') {
            setupWifiSection();
        }
        updateStatus();
    }

    function conectarBotonesPantallaSubmenu() {
        const btnEncender = document.getElementById('pantallaSubEncender');
        const btnApagar = document.getElementById('pantallaSubApagar');
        if (btnEncender) {
            btnEncender.onclick = () => {
                sistema.pantalla = true;
                updateStatus();
                showToast('Pantalla encendida', 'success');
            };
        }
        if (btnApagar) {
            btnApagar.onclick = () => {
                sistema.pantalla = false;
                updateStatus();
                showToast('Pantalla apagada', 'error');
            };
        }
        updateStatus();
    }

    // --- Lógica Específica para la Sección de WiFi ---
    function setupWifiSection() {
        const showAvailableBtn = $('showAvailableNetworksBtn');
        const showSavedBtn = $('showSavedNetworksBtn');
        const availableSection = $('availableNetworksSection');
        const savedSection = $('savedNetworksSection');

        const showAvailableNetworksListBtn = $('showAvailableNetworksListBtn');
        const showSavedNetworksListBtn = $('showSavedNetworksListBtn');
        const clearAllSavedNetworksBtn = $('clearAllSavedNetworksBtn');

        const availableTableBody = $('availableNetworksTableBody');
        const savedTableBody = $('savedNetworksTableBody');
        const noAvailableNetworksMsg = $('noAvailableNetworks');
        const noSavedNetworksMsg = $('noSavedNetworks');

        // Función para renderizar redes disponibles
        const renderAvailableNetworks = () => {
            // Datos simulados de redes disponibles
            sistema.wifi.availableNetworks = [
                { ssid: 'OFICINAABM', security: 'WPA2', intensity: 4 },
                { ssid: 'ABMMONITOREO', security: 'WPA', intensity: 3 },
                { ssid: 'IZZI-DDA9', security: 'WPA2', intensity: 3 },
                { ssid: 'ENSAMBLE', security: 'WEP', intensity: 2 },
                { ssid: 'INFINITUM9DAC', security: 'WPA2', intensity: 2 },
                { ssid: 'IZZI-A14F', security: 'WPA2', intensity: 1 },
                { ssid: 'Totalplay-2.4G-B6D0', security: 'WPA3', intensity: 4 },
                { ssid: 'Totalplay-EEA6', security: 'WPA2', intensity: 3 },
                { ssid: 'Club_Totalplay_WiFi', security: 'Abierta', intensity: 4 }
            ];

            if (sistema.wifi.availableNetworks.length === 0) {
                availableTableBody.innerHTML = '';
                noAvailableNetworksMsg.style.display = 'block';
                return;
            }

            let rowsHtml = '';
            sistema.wifi.availableNetworks.forEach(network => {
                rowsHtml += `
                    <tr>
                        <td>${network.ssid}</td>
                        <td>${network.security}</td>
                        <td>${getSignalIcon(network.intensity)}</td>
                        <td>
                            <button class="action-button table-button add-network-btn" data-ssid="${network.ssid}" data-security="${network.security}">Agregar Red</button>
                        </td>
                    </tr>
                `;
            });
            availableTableBody.innerHTML = rowsHtml;
            noAvailableNetworksMsg.style.display = 'none';

            // Conectar eventos para los botones de conectar red
            document.querySelectorAll('.add-network-btn').forEach(button => {
                button.onclick = (e) => {
                    const ssid = e.target.dataset.ssid;
                    const security = e.target.dataset.security;
                    if (security === 'Abierta') {
                        // For open networks, directly connect and save without asking for password
                        connectToWifi(ssid, '', false); // Pass empty string for password, and false for isFromSaved
                    } else {
                        showPasswordModal(ssid);
                    }
                };
            });
        };

        // Función para renderizar redes guardadas
        const renderSavedNetworks = () => {
            if (sistema.wifi.savedNetworks.length === 0) {
                savedTableBody.innerHTML = '<tr><td colspan="2" class="no-data-message">No hay redes guardadas.</td></tr>';
                noSavedNetworksMsg.style.display = 'block';
                return;
            }

            let rowsHtml = '';
            sistema.wifi.savedNetworks.forEach(network => {
                rowsHtml += `
                    <tr>
                        <td>${network.ssid}</td>
                        <td>
                            <button class="action-button table-button success-button connect-saved-wifi-btn" data-ssid="${network.ssid}" data-password="${network.password}">Conectar</button>
                            <button class="action-button table-button update-saved-wifi-btn" data-ssid="${network.ssid}">Actualizar</button>
                            <button class="action-button table-button danger-button delete-saved-wifi-btn" data-ssid="${network.ssid}">Eliminar</button>
                        </td>
                    </tr>
                `;
            });
            savedTableBody.innerHTML = rowsHtml;
            noSavedNetworksMsg.style.display = 'none';

            // Conectar eventos para los botones
            document.querySelectorAll('.delete-saved-wifi-btn').forEach(button => {
                button.onclick = (e) => {
                    const ssid = e.target.dataset.ssid;
                    deleteSavedWifi(ssid);
                };
            });

            document.querySelectorAll('.update-saved-wifi-btn').forEach(button => {
                button.onclick = (e) => {
                    const ssid = e.target.dataset.ssid;
                    const newPassword = prompt(`Ingrese la nueva contraseña para ${ssid}:`);
                    if (newPassword !== null) {
                        updateSavedWifi(ssid, newPassword);
                    }
                };
            });

            document.querySelectorAll('.connect-saved-wifi-btn').forEach(button => {
                button.onclick = (e) => {
                    const ssid = e.target.dataset.ssid;
                    const password = e.target.dataset.password;
                    showPasswordModal(ssid, true, password);
                };
            });
        };

        // Lógica para alternar entre secciones
        showAvailableBtn.onclick = () => {
            showAvailableBtn.classList.add('active');
            showSavedBtn.classList.remove('active');
            availableSection.classList.add('active-content');
            savedSection.classList.remove('active-content');
            availableTableBody.innerHTML = '<tr><td colspan="4" class="no-data-message">Presione el botón para Obtener las Redes Disponibles</td></tr>';
            noAvailableNetworksMsg.style.display = 'none';
        };

        showSavedBtn.onclick = () => {
            showSavedBtn.classList.add('active');
            showAvailableBtn.classList.remove('active');
            savedSection.classList.add('active-content');
            availableSection.classList.remove('active-content');
            savedTableBody.innerHTML = '<tr><td colspan="2" class="no-data-message">Presione el botón para Obtener las Redes Guardadas</td></tr>';
            noSavedNetworksMsg.style.display = 'none';
        };

        // Lógica para "Mostrar Redes Disponibles"
        showAvailableNetworksListBtn.onclick = () => {
            showToast('Buscando redes disponibles...', 'info');
            setTimeout(() => {
                renderAvailableNetworks();
                showToast(sistema.wifi.availableNetworks.length > 0 ? 
                    'Redes disponibles encontradas.' : 'No se encontraron redes disponibles.', 
                    sistema.wifi.availableNetworks.length > 0 ? 'success' : 'info');
            }, 1500);
        };

        // Lógica para "Mostrar Redes Guardadas"
        showSavedNetworksListBtn.onclick = () => {
            showToast('Cargando redes guardadas...', 'info');
            setTimeout(() => {
                renderSavedNetworks();
                showToast(sistema.wifi.savedNetworks.length > 0 ? 
                    'Redes guardadas cargadas.' : 'No hay redes guardadas.', 
                    sistema.wifi.savedNetworks.length > 0 ? 'success' : 'info');
            }, 1000);
        };

        // Lógica para "Borrar Todas las Redes Guardadas"
        clearAllSavedNetworksBtn.onclick = () => {
            if (confirm('¿Estás seguro de que deseas borrar todas las redes guardadas?')) {
                sistema.wifi.savedNetworks = [];
                renderSavedNetworks();
                showToast('Todas las redes guardadas han sido eliminadas.', 'info');
                if (sistema.wifi.connected) {
                    sistema.wifi.connected = false;
                    sistema.wifi.ssid = '';
                    sistema.wifi.password = '';
                    updateStatus();
                    showToast('Desconectado de la red actual.', 'warning');
                }
            }
        };

        // Función para conectar a una red Wi-Fi
        const connectToWifi = (ssid, password, isFromSaved = false) => {
            showToast(`Intentando conectar a ${ssid}...`, 'info');
            // Simulate connection delay
            setTimeout(() => {
                // In a real application, you'd send an AJAX request here to connect to the actual WiFi
                // For this simulation, we assume connection is always successful.

                sistema.wifi.ssid = ssid;
                sistema.wifi.password = password;
                sistema.wifi.connected = true;
                updateStatus();
                showToast(`Conectado a ${ssid} exitosamente!`, 'success');
                hidePasswordModal();

                // Only save if it's not coming from the saved networks list already
                // and it's not already in the saved networks list.
                if (!isFromSaved && !sistema.wifi.savedNetworks.some(net => net.ssid === ssid)) {
                    sistema.wifi.savedNetworks.push({ ssid, password });
                    renderSavedNetworks();
                    showToast(`Red "${ssid}" guardada automáticamente.`, 'info');
                }
            }, 1500); // Simulated 1.5 second connection time
        };

        // Función: Guardar una red Wi-Fi
        // This function is now mainly used internally by connectToWifi,
        // or if you want a separate "save" action.
        const saveWifiNetwork = (ssid, password) => {
            const existingNetworkIndex = sistema.wifi.savedNetworks.findIndex(net => net.ssid === ssid);
            if (existingNetworkIndex !== -1) {
                sistema.wifi.savedNetworks[existingNetworkIndex].password = password;
                showToast(`Contraseña de "${ssid}" actualizada.`, 'success');
            } else {
                sistema.wifi.savedNetworks.push({ ssid, password });
                showToast(`Red "${ssid}" agregada a las redes guardadas.`, 'success');
            }
            renderSavedNetworks(); // Re-render the saved networks list after saving
        };

        // Función para eliminar una red guardada
        const deleteSavedWifi = (ssid) => {
            if (confirm(`¿Estás seguro de que quieres eliminar la red "${ssid}" de las guardadas?`)) {
                sistema.wifi.savedNetworks = sistema.wifi.savedNetworks.filter(net => net.ssid !== ssid);
                renderSavedNetworks();
                showToast(`Red "${ssid}" eliminada de las guardadas.`, 'success');
                if (sistema.wifi.connected && sistema.wifi.ssid === ssid) {
                    sistema.wifi.connected = false;
                    sistema.wifi.ssid = '';
                    sistema.wifi.password = '';
                    updateStatus();
                    showToast('Desconectado de la red actual.', 'warning');
                }
            }
        };

        // Función para actualizar la contraseña de una red guardada
        const updateSavedWifi = (ssid, newPassword) => {
            const networkIndex = sistema.wifi.savedNetworks.findIndex(net => net.ssid === ssid);
            if (networkIndex !== -1) {
                if (newPassword.trim() === '') {
                    showToast('La contraseña no puede estar vacía.', 'error');
                    return;
                }
                sistema.wifi.savedNetworks[networkIndex].password = newPassword;
                renderSavedNetworks();
                showToast(`Contraseña de ${ssid} actualizada.`, 'success');
                if (sistema.wifi.connected && sistema.wifi.ssid === ssid) {
                    sistema.wifi.password = newPassword;
                }
            }
        };

        // Asegurarse de que la sección de disponibles esté activa al cargar
        showAvailableBtn.click();
    }

    // --- Event Handlers Principales ---

    menuToggle.onclick = () => {
        sidebar.classList.toggle('active');
        mainContent.classList.toggle('shifted');
        mainContent.style.marginLeft = sidebar.classList.contains('active') ? 
            FUNCIONES_SIDEBAR_WIDTH + 'px' : '0';
        menuToggle.classList.toggle('active');
        submenuOverlay.style.left = FUNCIONES_SIDEBAR_WIDTH + 'px';
        submenuOverlay.style.width = `calc(100vw - ${FUNCIONES_SIDEBAR_WIDTH}px)`;
        if (!sidebar.classList.contains('active')) {
            pantallaSubmenu.classList.remove('active');
            submenuOverlay.style.display = 'none';
        }
    };

    submenuOverlay.onclick = () => {
        if (pantallaSubmenu.classList.contains('active')) {
            pantallaSubmenu.classList.remove('active');
            submenuOverlay.style.display = 'none';
        }
    };

    navLinks.forEach(link => link.onclick = e => {
        e.preventDefault();
        const sectionId = link.dataset.section;
        if (sectionId !== 'funciones') {
            pantallaSubmenu.classList.remove('active');
            submenuOverlay.style.display = 'none';
        }
        if (sectionId === 'salir') {
            if (confirm('¿Deseas salir?')) {
                location.reload();
            }
            return;
        }
        document.querySelectorAll('main section').forEach(section => {
            section.classList.remove('active-section');
            section.style.display = 'none';
        });
        let targetSectionElement = null;
        if (sectionId === 'inicio') {
            targetSectionElement = $('inicio-section');
        } else if (sectionId === 'datos') {
            targetSectionElement = $('datos-section');
        } else if (sectionId === 'funciones') {
            pantallaSubmenu.classList.add('active');
            submenuOverlay.style.display = 'block';
            submenuOverlay.style.left = FUNCIONES_SIDEBAR_WIDTH + 'px';
            submenuOverlay.style.width = `calc(100vw - ${FUNCIONES_SIDEBAR_WIDTH}px)`;
            showSubsection('pantalla');
            pantallaSubmenuLinks.forEach(l => l.classList.remove('active'));
            const pantallaLink = pantallaSubmenu.querySelector('a[data-subsection="pantalla"]');
            if (pantallaLink) {
                pantallaLink.classList.add('active');
            }
            funcionesContentContainer.classList.add('active-section');
            funcionesContentContainer.style.display = 'block';
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
    });

    pantallaSubmenuLinks.forEach(link => link.onclick = e => {
        e.preventDefault();
        const subsectionId = link.dataset.subsection;
        pantallaSubmenuLinks.forEach(l => l.classList.remove('active'));
        link.classList.add('active');
        showSubsection(subsectionId);
    });

    // Event listeners para la sección de Datos
    if (guardarDatosButton) {
        guardarDatosButton.onclick = () => {
            const id = idDispositivoInput.value.trim();
            const canal = canalLoRaInput.value.trim();
            if (id && canal) {
                sistema.datos.id = id;
                sistema.datos.canal = canal;
                datosStatus.textContent = 'Datos guardados exitosamente.';
                datosStatus.style.color = '#2ecc71';
                showToast('Datos guardados exitosamente!', 'success');
                updateStatus();
            } else {
                datosStatus.textContent = 'Por favor, ingrese ID y Canal.';
                datosStatus.style.color = '#e74c3c';
                showToast('Error: ID y Canal son obligatorios.', 'error');
            }
        };
    }

    if (restablecerDatosButton) {
        restablecerDatosButton.onclick = () => {
            idDispositivoInput.value = '';
            canalLoRaInput.value = '';
            sistema.datos.id = '';
            sistema.datos.canal = '';
            datosStatus.textContent = ''; // Clear status message
            showToast('Datos restablecidos.', 'info');
            updateStatus();
        };
    }

    // Initial status update when the page loads
    updateStatus();

    // Set initial active section (e.g., 'inicio')
    const initialSectionLink = document.querySelector('#perro_siete ul li a[data-section="inicio"]');
    if (initialSectionLink) {
        initialSectionLink.click();
    }
});