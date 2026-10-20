# ☀️ Rastreador Solar Dual-Axis com ESP32

Este repositório reúne o código-fonte, esquemáticos e a documentação do meu Trabalho de Conclusão de Curso (TCC). O projeto consiste em um protótipo didático de um Rastreador Solar de dois eixos (Dual-Axis) com controle em malha fechada, visando a otimização da captação de energia fotovoltaica.

## 🚀 Principais Funcionalidades

- **Controle Preciso:** Algoritmo de controle PID discreto integrado para movimentos suaves dos servomotores.
- **Filtro Digital EMA:** Suavização de ruídos nas leituras dos sensores de luminosidade (LDRs).
- **Tolerância Dinâmica:** Adaptação automática da zona morta de acionamento baseada na luz ambiente.
- **Interface Web Embarcada:** Web Server local rodando no próprio ESP32 com visualização de telemetria em tempo real, gráficos dinâmicos e controle manual.
- **Autocalibração:** Sistema automático para compensação de *offsets* de hardware dos LDRs.

## 🛠️ Hardware Utilizado

- Microcontrolador ESP32
- 2x Servomotores MG996R (Eixo Azimutal e Vertical)
- 4x Sensores de Luminosidade (LDR) em matriz
- Display LCD 16x2 com módulo I2C
- Estrutura mecânica articulada (Modelos 3D disponíveis na pasta `/hardware`)

## ⚙️ Instalação e Uso

1. Clone este repositório: `git clone https://github.com/SEU_USUARIO/NOME_DO_REPO.git`
2. Abra o código localizado na pasta `src/` utilizando a Arduino IDE.
3. Instale as bibliotecas dependentes: `ESP32Servo`, `LiquidCrystal_I2C`.
4. Insira as suas credenciais de Wi-Fi no arquivo `.ino`.
5. Faça o upload para o ESP32 e acesse o endereço IP exibido no Display LCD através do seu navegador.

## 📁 Estrutura do Projeto

- `/src`: Código-fonte (`.ino` e cabeçalhos do servidor web).
- `/docs`: Guia de utilização, artigo do TCC e documentação teórica.
- `/hardware`: Esquemáticos elétricos e modelos 3D.
- `/images`: Fotos do protótipo e *screenshots* da interface web.

## 📄 Licença
Este projeto está sob a licença MIT. Veja o arquivo [LICENSE](LICENSE) para mais detalhes.
