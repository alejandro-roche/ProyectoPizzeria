# 🍕 Proyecto de Simulación de Pizzería Interactiva con MSP430G2553

Este proyecto, desarrollado como parte de la asignatura de Sistemas Electrónicos, consiste en una simulación interactiva de una pizzería de autoservicio, utilizando el microcontrolador **MSP430G2553** y su BoosterPack.

A través de un menú gráfico inspirado en cadenas como McDonald's o Burger King, los usuarios pueden personalizar pizzas, seleccionar ingredientes, y recibir estimaciones de precio y tiempo de espera en una interfaz gráfica mostrada en una pantalla LCD.

## 👥 Autores

- Alejandro Roche Aniento  
- Alfredo Zarazaga Montalbán  

## 📦 Recursos Utilizados

- Microcontrolador **MSP430G2553**
- BoosterPack con pantalla LCD y joystick
- Librerías:
  - `msp430.h`
  - `uart_STDIO.h`
  - `grlib.h`
  - `Crystalfontz128x128_ST7735.h`
  - `HAL_MSP430G2_Crystalfontz128x128_ST7735.h`
  - `stdio.h`

## ⚙️ Funcionalidades Principales

### 🧠 Lógica del Sistema

El sistema se basa en **dos máquinas de estados**:

#### 1. Máquina de Edición de Pizza
- Navegación por ingredientes con joystick.
- Añadir ingredientes si hay stock disponible.
- Cálculo dinámico de precio y tiempo de preparación.
- Gestión de puestos de trabajo y horno.
- Interacción por UART para:
  - Confirmar pedido (`d`)
  - Cancelar adición (`c`)
  - Recargar ingredientes (`r`)
  - Consultar precio medio (`q`)

#### 2. Máquina de Cocina
- Procesamiento automático de pedidos.
- Control de horno y turnos de entrega.
- Notificación al cliente cuando la pizza está lista.

### 📺 Interfaz Gráfica
- Menú interactivo en pantalla LCD.
- Indicadores visuales de stock, precios y turnos.
- Temporizador visual del tiempo restante para el pedido.

## 💾 Uso de Memoria

Se ha optimizado el uso de la memoria flash del microcontrolador para guardar los precios de cada pedido y mantener un funcionamiento fluido.

## 🛠️ Mejoras Futuras

- Representación visual más realista de los ingredientes sobre la pizza.
- Inclusión de logotipos o íconos por ingrediente.
- Optimización del uso de memoria.
- Expansión del sistema con nuevos periféricos o una interfaz más detallada.

## 📸 Galería

Se incluyen imágenes en la documentación (`MemoriaProyecto_ARA_AZM.pdf`) que muestran el funcionamiento paso a paso del sistema, desde la selección del ingrediente hasta la entrega del pedido.

## 📚 Conclusión

Este proyecto ha permitido aplicar conocimientos clave de electrónica, lógica digital, y programación de sistemas embebidos. A través de un ejemplo lúdico y funcional, se ha demostrado el uso eficiente del microcontrolador MSP430G2553 y la interacción con periféricos, todo enmarcado en un entorno amigable e intuitivo.

---

¡Gracias por visitar el repositorio!  
