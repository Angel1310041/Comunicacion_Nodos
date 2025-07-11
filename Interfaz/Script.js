document.addEventListener('DOMContentLoaded', () => {
// Elementos del DOM
const menuIcon = document.getElementById('menuIcon');
const sidebar = document.getElementById('sidebar');
const closeSidebarBtn = document.getElementById('closeSidebar');
const overlay = document.getElementById('overlay');
const inicioOption = document.getElementById('inicioOption');
const pantallaOption = document.getElementById('pantallaOption');
const homeSection = document.getElementById('homeSection');
const pantallaSettingsCard = document.getElementById('pantallaSettings');
const mainSectionTitle = document.getElementById('mainSectionTitle');
const currentStatusText = document.getElementById('currentStatus');
const pantallaVariableInput = document.getElementById('pantallaVariable');
const pantallaSelector = document.getElementById('pantallaSelector');
const saveScreenStateBtn = document.getElementById('saveScreenState');

// Estado
let pantallaEstado = false;
let pantallaNombreVariable = "pantallaActiva";

// 1. Manejo de UI y navegación
function manejarUI(accion, seccion = null, titulo = '') {
if (accion === 'abrirSidebar') {
sidebar.style.width = "250px";
overlay.classList.add('active');
} else if (accion === 'cerrarSidebar') {
sidebar.style.width = "0";
overlay.classList.remove('active');
} else if (accion === 'mostrarSeccion') {
homeSection.classList.add('hidden');
pantallaSettingsCard.classList.add('hidden');
seccion.classList.remove('hidden');
mainSectionTitle.textContent = titulo;
}
}

// 2. Actualizar estado visual de la pantalla
function actualizarEstadoPantalla(estado) {
pantallaEstado = estado;
currentStatusText.textContent = estado ? "Estado: ENCENDIDA" : "Estado: APAGADA";
currentStatusText.style.color = estado ? "#28a745" : "#dc3545";
pantallaSelector.value = estado.toString();
pantallaVariableInput.value = pantallaNombreVariable;
}

// 3. Comunicación con backend y guardar cambios
function guardarPantalla() {
const nuevoEstado = pantallaSelector.value === 'true';
const nuevoNombreVariable = pantallaVariableInput.value;
fetch('/pantalla', {
method: 'POST',
headers: { 'Content-Type': 'application/json' },
body: JSON.stringify({ estado: nuevoEstado, nombreVariable: nuevoNombreVariable }),
})
.then(response => response.json())
.then(data => {
pantallaNombreVariable = nuevoNombreVariable;
actualizarEstadoPantalla(nuevoEstado);
alert(data.mensaje || '¡Configuración de pantalla guardada con éxito!');
manejarUI('mostrarSeccion', homeSection, 'Inicio');
})
.catch(error => {
console.error('Error al comunicarse con el servidor:', error);
alert('Error de conexión al guardar la configuración.');
});
}

// 4. Inicialización y listeners
function inicializar() {
actualizarEstadoPantalla(false);
manejarUI('mostrarSeccion', homeSection, 'Inicio');

menuIcon.addEventListener('click', () => manejarUI('abrirSidebar'));
closeSidebarBtn.addEventListener('click', () => manejarUI('cerrarSidebar'));
overlay.addEventListener('click', () => manejarUI('cerrarSidebar'));
inicioOption.addEventListener('click', () => {
manejarUI('cerrarSidebar');
manejarUI('mostrarSeccion', homeSection, 'Inicio');
});
pantallaOption.addEventListener('click', () => {
manejarUI('cerrarSidebar');
manejarUI('mostrarSeccion', pantallaSettingsCard, 'Configuración de Pantalla');
pantallaSelector.value = pantallaEstado.toString();
pantallaVariableInput.value = pantallaNombreVariable;
});
saveScreenStateBtn.addEventListener('click', guardarPantalla);
}

inicializar();
});