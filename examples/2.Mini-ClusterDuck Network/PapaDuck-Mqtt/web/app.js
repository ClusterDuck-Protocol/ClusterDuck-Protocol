// Replace these with your actual MQTT broker details
const mqttBrokerUrl = "wss://test.mosquitto.org:8081";
const clientId = "web_client_" + parseInt(Math.random() * 100, 10);

// Connect to the broker
const client = mqtt.connect(mqttBrokerUrl, {
    clientId: clientId
});

client.on('connect', function () {
    console.log("Connected to MQTT broker");
    client.subscribe("hub/event"); // Subscribe to the topic PapaDuck publishes to
});

client.on('message', function (topic, message) {
    // message is Buffer, convert to string
    var msgString = message.toString();
    console.log("Message arrived: " + msgString);

    // Log the entire message object
    console.log("Message object: ", message);

    try {
        var msg = JSON.parse(msgString);
        onMessageArrived(topic, msg);
    } catch (e) {
        console.error("Error parsing message: ", e);
    }
});

client.on('error', function (error) {
    console.log('Error: ', error);
});

client.on('offline', function () {
    console.log('Client is offline');
});

client.on('reconnect', function () {
    console.log('Client is reconnecting');
});

function onMessageArrived(topic, msg) {
    console.log("Message arrived: ", msg);

    // Get the table body where you want to insert the rows
    var tbody = document.getElementById('messageTable').getElementsByTagName('tbody')[0];

    // Create a new row and cells
    var newRow = document.createElement('tr');
    var mqttTopicCell = document.createElement('td');
    var fromCell = document.createElement('td');
    var toCell = document.createElement('td');
    var cdpTopicCell = document.createElement('td');
    var responseCell = document.createElement('td');
    var payloadCell = document.createElement('td');
    

    // Set the text of the cells
    mqttTopicCell.innerText = topic; // MQTT topic
    fromCell.innerText = msg.from;
    toCell.innerText = msg.to;
    cdpTopicCell.innerText = msg.messageTopic; // CDP topic
    responseCell.innerText = msg.responseExpected; // Whether a response is required
    payloadCell.innerText = JSON.stringify(msg.payload);

    // Append the cells to the row
    newRow.appendChild(mqttTopicCell);
    newRow.appendChild(fromCell);
    newRow.appendChild(toCell);
    newRow.appendChild(cdpTopicCell);
    newRow.appendChild(responseCell);
    newRow.appendChild(payloadCell);

    // Append the row to the table body
    tbody.appendChild(newRow);
}

function publishMessage() {
    const mqttTopic = document.getElementById("mqttTopic").value;
    const duckId = document.getElementById("duckId").value;
    const cdpTopic = document.getElementById("cdpTopic").value;
    const messageBody = document.getElementById("messageBody").value;

    const payload = JSON.stringify({
        duckId: duckId,
        cdpTopic: cdpTopic,
        message: messageBody
    });

    client.publish(mqttTopic, payload);
}
