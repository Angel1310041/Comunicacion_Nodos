document.addEventListener('DOMContentLoaded', () => {
    // Selectores (manteniendo IDs perro_XX)
    const $ = id => document.getElementById(id);
    const sidebar = $('perro_cinco');
    const menuToggle = $('perro_dos'); // El icono de las tres líneas
    const mainContent = $('perro_once');
    const navLinks = document.querySelectorAll('#perro_siete ul li a');
    const screenStatusSummary = $('perro_dieciocho');
    const screenStatusDetail = $('perro_veinticuatro');
    const turnOnButton = $('perro_veinticinco');
    const turnOffButton = $('perro_veintiseis');


    // Estado del sistema (considerando los elementos existentes)
    let sistema = {
        pantalla: false,
        serialConnected: false, // Mantener para futura expansión o si los elementos existen en otro lugar
        port: null // Mantener para futura expansión
    };

    // Actualizar interfaz (ajustado para los elementos existentes)
    const updateStatus = () => {
        screenStatusSummary.textContent = sistema.pantalla ? 'ENCENDIDA' : 'APAGADA';
        screenStatusDetail.textContent = sistema.pantalla ? 'Encendida' : 'Apagada';
        // if (statusIndicator) { // Verificar si el elemento existe antes de intentar usarlo
        //     statusIndicator.textContent = sistema.serialConnected ? 'Conectado' : 'Desconectado';
        //     statusIndicator.className = sistema.serialConnected ? 'connected' : 'disconnected';
        // }
    };

    // WebSerial API (Mantenido, pero los elementos HTML asociados no están en el código que proporcionaste)
    const connectSerial = async () => {
        try {
            sistema.port = await navigator.serial.requestPort();
            await sistema.port.open({ baudRate: 115200 });
            sistema.serialConnected = true;

            const reader = sistema.port.readable.getReader();
            while (sistema.serialConnected) {
                const { value, done } = await reader.read();
                if (done) break;
                const text = new TextDecoder().decode(value);
                // if (consoleOutput) { // Verificar si el elemento existe
                //     consoleOutput.innerHTML += `> ${text}<br>`;
                //     consoleOutput.scrollTop = consoleOutput.scrollHeight;
                // }
            }
        } catch (error) {
            console.error('Error Serial:', error);
            alert(`Error de conexión: ${error}`);
        }
        updateStatus();
    };

    const disconnectSerial = () => {
        if (sistema.port) {
            sistema.serialConnected = false;
            sistema.port.close();
            sistema.port = null;
        }
        updateStatus();
    };

    const sendCommand = async (command) => {
        if (!sistema.serialConnected) {
            alert("Primero conecta el puerto serial");
            return;
        }

        const writer = sistema.port.writable.getWriter();
        await writer.write(new TextEncoder().encode(command + '\n'));
        writer.release();
        // if (consoleOutput) { // Verificar si el elemento existe
        //     consoleOutput.innerHTML += `< ${command}<br>`;
        //     consoleOutput.scrollTop = consoleOutput.scrollHeight;
        // }
    };

    // Control de pantalla
    if (turnOnButton) {
        turnOnButton.onclick = () => {
            sendCommand('pantalla on');
            sistema.pantalla = true;
            updateStatus();
        };
    }

    if (turnOffButton) {
        turnOffButton.onclick = () => {
            sendCommand('pantalla off');
            sistema.pantalla = false;
            updateStatus();
        };
    }

    // Enviar mensaje personalizado (comentado ya que los elementos no están en el HTML)
    // if (sendButton) {
    //     sendButton.onclick = () => {
    //         const message = messageInput.value.trim();
    //         if (message) {
    //             sendCommand(message);
    //             messageInput.value = '';
    //         }
    //     };
    // }

    // Conexión Serial (comentado ya que los elementos no están en el HTML)
    // if (connectButton) {
    //     connectButton.onclick = () => {
    //         if (sistema.serialConnected) {
    //             disconnectSerial();
    //         } else {
    //             connectSerial();
    //         }
    //     };
    // }

    // Toggle sidebar
    menuToggle.onclick = () => {
        sidebar.classList.toggle('active');
        mainContent.classList.toggle('shifted');
        menuToggle.classList.toggle('active'); // <-- AÑADE ESTA LÍNEA
    };

    // Navegación
    navLinks.forEach(link => link.onclick = e => {
        e.preventDefault();
        const sectionId = link.dataset.section;

        if (sectionId === 'salir') {
            if (confirm('¿Deseas salir y reiniciar la tarjeta?')) {
                disconnectSerial();
                location.reload();
            }
            return;
        }

        // Cambiar sección activa
        // Es mejor seleccionar las secciones por una clase común, no por IDs fijos.
        // Asumiendo que 'perro_doce' y 'perro_diecinueve' son las secciones de contenido.
        document.querySelectorAll('main section').forEach(section => { // <-- CAMBIO AQUÍ
            section.classList.remove('active-section');
        });

        // Asegúrate de que las secciones tengan una clase común, por ejemplo, 'content-section'.
        // Si no, puedes mantener la lógica original o mejorarla con un mapeo de data-section a IDs.
        const targetSectionId = sectionId === 'inicio' ? 'perro_doce' : (sectionId === 'pantalla' ? 'perro_diecinueve' : null);
        if (targetSectionId) {
            $(targetSectionId).classList.add('active-section');
        }


        // Cambiar enlace activo
        navLinks.forEach(nl => nl.classList.remove('active'));
        link.classList.add('active');

        // Cerrar sidebar después de navegar
        sidebar.classList.remove('active');
        mainContent.classList.remove('shifted');
        menuToggle.classList.remove('active'); // <-- AÑADE ESTA LÍNEA para resetear el icono
    });

    // Estado inicial
    updateStatus();
    // Asegúrate de que solo la sección de inicio esté activa al cargar
    $('perro_doce').classList.add('active-section');
    $('perro_diecinueve').classList.remove('active-section'); // Asegúrate de que la otra sección esté oculta
    document.querySelector('#perro_ocho a').classList.add('active'); // Activa el enlace de "Inicio"
});