    document.addEventListener('DOMContentLoaded', () => {
    // Selectores
    const $ = id => document.getElementById(id);
    const sidebar = $('perro_cinco');
    const menuToggle = $('perro_dos');
    const mainContent = $('perro_doce');
    const navLinks = document.querySelectorAll('#perro_siete ul li a');

    // Elementos de estado
    const screenStatusSummary = $('perro_diecinueve');
    const screenStatusDetail = $('perro_veintisiete');
    const wifiStatus = $('perro_veintiuno');

    // Botones Pantalla
    const turnOnButton = $('perro_veintiocho');
    const turnOffButton = $('perro_veintinueve');

    // WiFi Section Elements
    const showAvailableNetworksButton = $('showAvailableNetworks');
    const showSavedNetworksButton = $('showSavedNetworks');
    const availableNetworksSection = $('availableNetworksSection');
    const savedNetworksSection = $('savedNetworksSection');
    const getAvailableNetworksButton = $('getAvailableNetworks');
    const getSavedNetworksButton = $('getSavedNetworks');
    const clearAllSavedNetworksButton = $('clearAllSavedNetworks');
    const availableNetworksTableBody = $('availableNetworksTable').querySelector('tbody');
    const savedNetworksTableBody = $('savedNetworksTable').querySelector('tbody');
    const loadingAvailable = $('loadingAvailable');
    const loadingSaved = $('loadingSaved');
    const noAvailableNetworksMessage = $('noAvailableNetworks');
    const noSavedNetworksMessage = $('noSavedNetworks');
    const availableNetworksTable = $('availableNetworksTable');
    const savedNetworksTable = $('savedNetworksTable');

    // Toast Container
    const toastContainer = $('toastContainer');

    // Estado del sistema (Simulated for frontend demo)
    let sistema = {
    pantalla: false,
    wifi: {
    ssid: '',
    password: '',
    connected: false
    }
    };

    // Simulated Data for Networks
    let availableNetworksData = [];
    let savedNetworksData = [
    { ssid: 'OFCINAABM', password: 'T3mp0r4lABM123', security: 3, intensity: 80 },
    { ssid: 'CasaWifi', password: 'password123', security: 3, intensity: 95 }
    ];

    // --- Helper Functions ---

    // Function to show toast notifications
    const showToast = (message, type = 'info') => {
    const toast = document.createElement('div');
    toast.classList.add('toast', type);
    toast.textContent = message;
    toastContainer.appendChild(toast);

    setTimeout(() => {
    toast.style.opacity = '0';
    toast.style.transform = 'translateY(-20px)';
    toast.addEventListener('transitionend', () => toast.remove());
    }, 3000); // Toast disappears after 3 seconds
    };

    // Function to update main system status display
    const updateStatus = () => {
    // Update screen status
    screenStatusSummary.textContent = sistema.pantalla ? 'ENCENDIDA' : 'APAGADA';
    screenStatusDetail.textContent = sistema.pantalla ? 'Encendida' : 'Apagada';

    // Update WiFi status
    const wifiText = sistema.wifi.connected && sistema.wifi.ssid ?
    `Conectado a: ${sistema.wifi.ssid}` : 'No configurado / Desconectado';
    wifiStatus.textContent = wifiText;
    wifiStatus.style.color = sistema.wifi.connected ? '#2ecc71' : '#e74c3c';
    };

    // Function to get Wi-Fi signal icon and class
    const getSignalIcon = (intensity) => {
    let iconClass = 'fas fa-wifi signal-icon';
    if (intensity > 75) {
    iconClass += ''; // Strong signal (no extra class)
    } else if (intensity > 50) {
    iconClass += ' medium'; // Medium signal
    } else {
    iconClass += ' weak'; // Weak signal
    }
    return `<i class="${iconClass}"></i>`;
    };

    // --- Render Functions ---

    // Render Available Networks
    const renderAvailableNetworks = () => {
    availableNetworksTableBody.innerHTML = '';
    if (availableNetworksData.length === 0) {
    noAvailableNetworksMessage.style.display = 'block';
    availableNetworksTable.style.display = 'none';
    } else {
    noAvailableNetworksMessage.style.display = 'none';
    availableNetworksTable.style.display = 'table';
    availableNetworksData.forEach(network => {
    const row = availableNetworksTableBody.insertRow();
    row.insertCell().textContent = network.ssid;
    row.insertCell().textContent = network.security;
    row.insertCell().innerHTML = getSignalIcon(network.intensity);
    row.insertCell().textContent = 'N/A'; // Placeholder for password column
    });
    }
    };

    // Render Saved Networks
    const renderSavedNetworks = () => {
    savedNetworksTableBody.innerHTML = '';
    if (savedNetworksData.length === 0) {
    noSavedNetworksMessage.style.display = 'block';
    savedNetworksTable.style.display = 'none';
    } else {
    noSavedNetworksMessage.style.display = 'none';
    savedNetworksTable.style.display = 'table';
    savedNetworksData.forEach(network => {
    const row = savedNetworksTableBody.insertRow();
    row.insertCell().textContent = network.ssid;
    const actionCell = row.insertCell();

    const deleteButton = document.createElement('button');
    deleteButton.textContent = 'Eliminar';
    deleteButton.classList.add('table-button', 'delete-button');
    deleteButton.onclick = (e) => {
    e.stopPropagation(); // Prevent row click
    deleteSavedNetwork(network.ssid);
    };
    actionCell.appendChild(deleteButton);
    });
    }
    };

    // --- Event Handlers ---

    // Control de pantalla
    if (turnOnButton) {
    turnOnButton.onclick = () => {
    sistema.pantalla = true;
    updateStatus();
    showToast('Pantalla encendida', 'success');
    console.log('Pantalla encendida');
    // Add fetch call to ESP32 API here
    };
    }

    if (turnOffButton) {
    turnOffButton.onclick = () => {
    sistema.pantalla = false;
    updateStatus();
    showToast('Pantalla apagada', 'error');
    console.log('Pantalla apagada');
    // Add fetch call to ESP32 API here
    };
    }

    // Toggle sidebar
    menuToggle.onclick = () => {
    sidebar.classList.toggle('active');
    mainContent.classList.toggle('shifted');
    menuToggle.classList.toggle('active');
    };

    // Navigation
    navLinks.forEach(link => link.onclick = e => {
    e.preventDefault();
    const sectionId = link.dataset.section;

    if (sectionId === 'salir') {
    if (confirm('¿Deseas salir?')) {
    location.reload(); // Simple reload for "exit"
    }
    return;
    }

    // Hide all sections
    document.querySelectorAll('main section').forEach(section => {
    section.classList.remove('active-section');
    });

    // Show the corresponding section
    const targetSectionId =
    sectionId === 'inicio' ? 'perro_trece' :
    sectionId === 'pantalla' ? 'perro_veintidos' :
    sectionId === 'wifi' ? 'perro_treinta' : null;

    if (targetSectionId) {
    $(targetSectionId).classList.add('active-section');
    }

    // Change active link
    navLinks.forEach(nl => nl.classList.remove('active'));
    link.classList.add('active');

    // Close sidebar
    sidebar.classList.remove('active');
    mainContent.classList.remove('shifted');
    menuToggle.classList.remove('active');
    });

    // --- WiFi Configuration Logic ---

    // Switch between Available and Saved Networks tabs
    showAvailableNetworksButton.onclick = () => {
    showAvailableNetworksButton.classList.add('active');
    showSavedNetworksButton.classList.remove('active');
    availableNetworksSection.classList.add('active');
    savedNetworksSection.classList.remove('active');
    // Clear saved networks display when switching to available
    savedNetworksTableBody.innerHTML = '';
    noSavedNetworksMessage.style.display = 'block';
    savedNetworksTable.style.display = 'none';
    // Ensure "Presione el botón para Obtener las Redes Disponibles" message is shown
    noAvailableNetworksMessage.style.display = 'block';
    availableNetworksTable.style.display = 'none';
    availableNetworksData = []; // Clear current displayed available networks
    };

    showSavedNetworksButton.onclick = () => {
    showSavedNetworksButton.classList.add('active');
    showAvailableNetworksButton.classList.remove('active');
    savedNetworksSection.classList.add('active');
    availableNetworksSection.classList.remove('active');
    // Clear available networks display when switching to saved
    availableNetworksTableBody.innerHTML = '';
    noAvailableNetworksMessage.style.display = 'block';
    availableNetworksTable.style.display = 'none';
    // Ensure "Presione el botón para Obtener las Redes Guardadas" message is shown
    noSavedNetworksMessage.style.display = 'block';
    savedNetworksTable.style.display = 'none';
    savedNetworksData = savedNetworksData.slice(); // Copy to trigger a "refresh" if desired, or just re-render
    renderSavedNetworks(); // Render existing saved data
    };

    // Fetch and display Available Networks (Simulated)
    getAvailableNetworksButton.onclick = () => {
    loadingAvailable.style.display = 'block';
    noAvailableNetworksMessage.style.display = 'none';
    availableNetworksTable.style.display = 'none';
    availableNetworksData = []; // Clear previous data

    setTimeout(() => { // Simulate API call delay
    availableNetworksData = [
    { ssid: 'OFCINAABM', security: 'WPA2', intensity: 85 },
    { ssid: 'ABMMONITOREO', security: 'WPA2', intensity: 70 },
    { ssid: 'IZZI-DDA9', security: 'WPA', intensity: 60 },
    { ssid: 'ENSAMBLE', security: 'OPEN', intensity: 45 },
    { ssid: 'INFINITUM9DAC', security: 'WPA2', intensity: 90 },
    { ssid: 'Totalplay-2.4G-B6D0', security: 'WPA3', intensity: 78 },
    { ssid: 'Club_Totalplay_WiFi', security: 'OPEN', intensity: 55 }
    ];
    loadingAvailable.style.display = 'none';
    renderAvailableNetworks();
    showToast('Redes disponibles actualizadas', 'success');
    }, 1500); // 1.5 second delay
    };

    // Fetch and display Saved Networks (Simulated)
    getSavedNetworksButton.onclick = () => {
    loadingSaved.style.display = 'block';
    noSavedNetworksMessage.style.display = 'none';
    savedNetworksTable.style.display = 'none';

    setTimeout(() => { // Simulate API call delay
    loadingSaved.style.display = 'none';
    renderSavedNetworks();
    showToast('Redes guardadas actualizadas', 'success');
    }, 1000); // 1 second delay
    };

    // Clear all Saved Networks
    clearAllSavedNetworksButton.onclick = () => {
    if (confirm('¿Estás seguro de que quieres borrar TODAS las redes guardadas?')) {
    savedNetworksData = [];
    renderSavedNetworks();
    showToast('Todas las redes guardadas han sido eliminadas', 'success');
    // Here you would send a command to your ESP32 to clear saved networks
    }
    };

    // Delete Saved Network
    const deleteSavedNetwork = (ssidToDelete) => {
    if (confirm(`¿Estás seguro de que quieres eliminar la red "${ssidToDelete}" de las guardadas?`)) {
    const initialLength = savedNetworksData.length;
    savedNetworksData = savedNetworksData.filter(net => net.ssid !== ssidToDelete);
    if (savedNetworksData.length < initialLength) {
    renderSavedNetworks();
    showToast(`Red "${ssidToDelete}" eliminada`, 'success');
    // If the deleted network was the one currently connected, update status
    if (sistema.wifi.ssid === ssidToDelete) {
    sistema.wifi.ssid = '';
    sistema.wifi.password = '';
    sistema.wifi.connected = false;
    updateStatus();
    }
    } else {
    showToast(`Error: No se encontró la red "${ssidToDelete}".`, 'error');
    }
    }
    };

    // Initial state setup
    updateStatus();
    $('perro_trece').classList.add('active-section'); // Show Inicio section by default
    document.querySelector('#perro_ocho a').classList.add('active'); // Activate Inicio link

    // Automatically trigger 'Obtener Redes Disponibles' when WiFi section is first shown.
    const wifiNavLink = document.querySelector('a[data-section="wifi"]');
    if (wifiNavLink) {
    wifiNavLink.addEventListener('click', () => {
    // Only fetch if availableNetworksData is empty and available section is active
    if (availableNetworksData.length === 0 && availableNetworksSection.classList.contains('active')) {
    getAvailableNetworksButton.click(); // Simulate click on the "Mostrar Redes Disponibles" button
    }
    });
    }
    });