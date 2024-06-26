Comando para executar a aplicação
- Ativar o Zephyr (Na minha máquina source workspace/zephyrproject/.venv/bin/activate
- Ir até a pasta zephyrproject/zephyr
- Rodar o comando west build -b esp32_devkitc_wroom/esp32/procpu --sysbuild samples/basic/esp32_led --pristine
(Para buildar a aplicação para o esp32)
- Depois para gravar na placa é west flash
- E para ver o terminal é west espressif monitor

- A placa utilizada foi uma ESP-WROOM-32
- Além do SDK do zephyr instalamos "west blobs fetch hal_espressif"
  Para obter algumas funcionalidades do SDK do ESP

  Uma aplicação simples de um semaforo, onde a thread principal é a do semaforo e a secundaria é o pedestre
  Ele fica ciclando o semaforo de 3 estagios e quando solicitada a passagem ele pisca o sinal vermelho

  Aplicações futuras seria controlar através de cameras o trafego de carros para otimizar a travessia de pedestres, ou controle de um cruzamento

  
