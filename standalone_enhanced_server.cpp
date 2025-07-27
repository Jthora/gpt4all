#include <QCoreApplicati            qDebug() << "   Health check: http://localhost:" << port << "/";
            qDebug() << "   Models list:  http://localhost:" << port << "/v1/models";
            qDebug() << "   Chat API:     http://localhost:" << port << "/v1/chat/completions";
            qDebug() << "";
            qDebug() << "🧪 Test with curl:";
            qDebug() << "   curl http://localhost:" << port << "/";
            qDebug() << "";
            qDebug() << "🚗 Try the car parts question:";
            qDebug() << "   curl -X POST http://localhost:" << port << "/v1/chat/completions \\";
            qDebug() << "        -H \"Content-Type: application/json\" \\";
            qDebug() << "        -d '{\"messages\": [{\"role\": \"user\", \"content\": \"Create a categorized list of all the parts in a car\"}]}'\";e <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QDebug>
#include <QDateTime>

class SimpleHTTPServer : public QObject {
    Q_OBJECT

public:
    SimpleHTTPServer(QObject *parent = nullptr) : QObject(parent) {
        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection, this, &SimpleHTTPServer::handleNewConnection);
    }

    bool startServer(quint16 port) {
        if (m_server->listen(QHostAddress::Any, port)) {
            qDebug() << "🚀 GPT4All-Enhanced HTTP Server started on port" << port;
            qDebug() << "🔗 Available endpoints:";
            qDebug() << "   Health check: http://localhost:" << port << "/";
            qDebug() << "   Models list:  http://localhost:" << port << "/v1/models";
            qDebug() << "   Chat API:     http://localhost:" << port << "/v1/chat/completions";
            qDebug() << "";
            qDebug() << "🧪 Test with curl:";
            qDebug() << "   curl http://localhost:" << port << "/";
            qDebug() << "";
            qDebug() << "🚗 Try the car parts question:";
            qDebug() << "   curl -X POST http://localhost:" << port << "/v1/chat/completions \\";
            qDebug() << "        -H \"Content-Type: application/json\" \\";
            qDebug() << "        -d '{\"messages\": [{\"role\": \"user\", \"content\": \"Create a categorized list of all the parts in a car\"}]}'";
            return true;
        } else {
            qDebug() << "Failed to start server on port" << port;
            return false;
        }
    }

private slots:
    void handleNewConnection() {
        QTcpSocket *client = m_server->nextPendingConnection();
        connect(client, &QTcpSocket::readyRead, [this, client]() {
            QByteArray data = client->readAll();
            processRequest(client, data);
        });
        connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
    }

private:
    void processRequest(QTcpSocket *client, const QByteArray &data) {
        QString request = QString::fromUtf8(data);
        QStringList lines = request.split("\r\n");
        
        if (lines.isEmpty()) return;
        
        QStringList requestLine = lines[0].split(" ");
        if (requestLine.size() < 2) return;
        
        QString method = requestLine[0];
        QString path = requestLine[1];
        
        qDebug() << "📥 Received" << method << path;
        
        QByteArray response;
        
        if (method == "GET" && path == "/") {
            response = createHealthResponse();
        } else if (method == "GET" && path == "/v1/models") {
            response = createModelsResponse();
        } else if (method == "POST" && path == "/v1/chat/completions") {
            // Extract JSON body
            int bodyStart = request.indexOf("\r\n\r\n");
            QString jsonBody;
            if (bodyStart != -1) {
                jsonBody = request.mid(bodyStart + 4);
            }
            response = createChatResponse(jsonBody);
        } else {
            response = createNotFoundResponse();
        }
        
        client->write(response);
        client->flush();
        client->disconnectFromHost();
    }
    
    QByteArray createHealthResponse() {
        QJsonObject health;
        health["status"] = "ok";
        health["server"] = "GPT4All-Enhanced-Simple";
        health["version"] = "1.0";
        health["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        health["message"] = "GPT4All HTTP server running with enhanced automotive knowledge!";
        
        return createHttpResponse(200, QJsonDocument(health).toJson(QJsonDocument::Compact));
    }
    
    QByteArray createModelsResponse() {
        QJsonArray models;
        
        QJsonObject model;
        model["id"] = "gpt4all-enhanced";
        model["object"] = "model";
        model["created"] = 1686935002;
        model["owned_by"] = "nomic-ai";
        models.append(model);
        
        QJsonObject response;
        response["object"] = "list";
        response["data"] = models;
        
        return createHttpResponse(200, QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
    
    QByteArray createChatResponse(const QString &jsonBody) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonBody.toUtf8(), &error);
        
        if (error.error != QJsonParseError::NoError) {
            QJsonObject errorObj;
            errorObj["error"] = "Invalid JSON";
            return createHttpResponse(400, QJsonDocument(errorObj).toJson(QJsonDocument::Compact));
        }
        
        QJsonObject request = doc.object();
        QJsonArray messages = request["messages"].toArray();
        
        QString userPrompt;
        for (const auto &msgVal : messages) {
            QJsonObject msg = msgVal.toObject();
            if (msg["role"].toString() == "user") {
                userPrompt = msg["content"].toString();
            }
        }
        
        if (userPrompt.isEmpty()) {
            QJsonObject errorObj;
            errorObj["error"] = "No user message found";
            return createHttpResponse(400, QJsonDocument(errorObj).toJson(QJsonDocument::Compact));
        }
        
        qDebug() << "🤖 Processing prompt:" << userPrompt.left(50) << "...";
        
        QString aiResponse = generateAIResponse(userPrompt);
        
        // Create OpenAI-compatible response
        QJsonObject responseObj;
        responseObj["id"] = QString("chatcmpl-%1").arg(QDateTime::currentMSecsSinceEpoch());
        responseObj["object"] = "chat.completion";
        responseObj["created"] = QDateTime::currentSecsSinceEpoch();
        responseObj["model"] = "gpt4all-enhanced";
        
        QJsonArray choices;
        QJsonObject choice;
        choice["index"] = 0;
        choice["finish_reason"] = "stop";
        
        QJsonObject message;
        message["role"] = "assistant";
        message["content"] = aiResponse;
        choice["message"] = message;
        
        choices.append(choice);
        responseObj["choices"] = choices;
        
        QJsonObject usage;
        usage["prompt_tokens"] = userPrompt.split(" ").size();
        usage["completion_tokens"] = aiResponse.split(" ").size();
        usage["total_tokens"] = usage["prompt_tokens"].toInt() + usage["completion_tokens"].toInt();
        responseObj["usage"] = usage;
        
        return createHttpResponse(200, QJsonDocument(responseObj).toJson(QJsonDocument::Compact));
    }
    
    QString generateAIResponse(const QString &userPrompt) {
        QString lowerPrompt = userPrompt.toLower();
        
        // Enhanced automotive responses
        if (lowerPrompt.contains("car") && (lowerPrompt.contains("parts") || lowerPrompt.contains("list") || lowerPrompt.contains("component"))) {
            return R"(Here's a comprehensive categorized list of all the major parts in a car:

**Engine System:**
- Engine block and cylinders (4, 6, 8, or more cylinders)
- Pistons, connecting rods, and crankshaft
- Camshaft and timing belt/chain
- Cylinder heads, valves (intake/exhaust), and valve springs
- Fuel injectors and fuel rail
- Spark plugs and ignition coils
- Oil pump, oil filter, and oil pan
- Radiator, water pump, and cooling system
- Air intake manifold and throttle body
- Exhaust manifold and catalytic converter

**Transmission & Drivetrain:**
- Transmission (manual, automatic, or CVT)
- Clutch assembly (manual) or torque converter (automatic)
- Driveshaft and universal joints
- Differential (front, rear, or both)
- Axles and CV joints
- Transfer case (AWD/4WD vehicles)

**Electrical System:**
- Battery and battery terminals
- Alternator and voltage regulator
- Starter motor and starter solenoid
- Ignition system and ECU (Engine Control Unit)
- Wiring harness and fuse box
- Relays and sensors (dozens of them)
- Lights: headlights, taillights, brake lights, turn signals
- Dashboard instruments and displays

**Braking System:**
- Brake pads and brake rotors (front/rear)
- Brake calipers and brake cylinders
- Master cylinder and brake booster
- Brake fluid reservoir and brake lines
- ABS system (Anti-lock Braking System)
- Parking brake assembly

**Suspension & Steering:**
- Shock absorbers or struts
- Springs (coil springs, leaf springs, or air springs)
- Control arms (upper and lower)
- Steering wheel, steering column, and steering rack
- Tie rods and ball joints
- Sway bars and bushings
- Power steering pump and fluid

**Body & Interior:**
- Chassis/frame structure
- Body panels: doors, hood, trunk, fenders
- Bumpers (front and rear)
- Windshield and windows
- Seats (driver, passenger, rear)
- Dashboard and center console
- Air conditioning system (compressor, evaporator, condenser)
- Heating system and HVAC controls
- Interior trim, carpeting, and upholstery
- Airbags (driver, passenger, side, curtain)

**Wheels & Tires:**
- Wheels/rims (steel or alloy)
- Tires (all-season, summer, winter)
- Wheel bearings and hub assemblies
- Lug nuts and wheel bolts
- Tire pressure monitoring sensors (TPMS)

**Exhaust System:**
- Exhaust manifold
- Catalytic converter
- Muffler and resonator
- Exhaust pipes and tailpipe
- Oxygen sensors

This represents the core systems that make a modern car function safely and efficiently!)";
        }
        else if (lowerPrompt.contains("engine") && lowerPrompt.contains("car")) {
            return R"(Car engines are fascinating pieces of engineering! Modern automotive engines are typically 4-stroke internal combustion engines that convert fuel into mechanical energy through a precise cycle:

**The Four Strokes:**
1. **Intake Stroke** - The piston moves down, creating vacuum that draws the air/fuel mixture through the intake valve
2. **Compression Stroke** - Both valves close, piston moves up, compressing the mixture to about 1/10th original volume
3. **Power Stroke** - Spark plug ignites the compressed mixture, explosion forces piston down
4. **Exhaust Stroke** - Exhaust valve opens, piston moves up, pushing burned gases out

**Key Engine Components Working Together:**
- **Cylinders** contain the pistons and combustion
- **Pistons** transfer combustion force to the crankshaft
- **Connecting rods** link pistons to the crankshaft
- **Crankshaft** converts linear piston motion to rotational motion
- **Camshaft** controls valve timing via the timing belt/chain
- **Valves** control air/fuel intake and exhaust output

**Engine Types:**
- Inline engines (I4, I6) - cylinders in a straight line
- V engines (V6, V8) - cylinders in V formation
- Boxer/Flat engines - cylinders horizontally opposed

Modern engines also include sophisticated fuel injection, variable valve timing, turbocharging, and computer-controlled ignition timing for optimal performance and efficiency!)";
        }
        else if (lowerPrompt.contains("brake") || lowerPrompt.contains("stop")) {
            return R"(Car braking systems are critical safety components designed to convert kinetic energy into heat energy through friction:

**How Car Brakes Work:**
When you press the brake pedal, you're activating a hydraulic system that multiplies your foot force. Here's the process:

1. **Brake Pedal** - Your input force
2. **Master Cylinder** - Converts pedal force to hydraulic pressure
3. **Brake Fluid** - Incompressible liquid that transfers force through brake lines
4. **Brake Calipers/Cylinders** - Convert hydraulic pressure back to mechanical force
5. **Brake Pads/Shoes** - Create friction against rotors/drums to stop wheels

**Types of Brakes:**
- **Disc Brakes** (front wheels, often rear too) - Brake pads squeeze rotating disc (rotor)
- **Drum Brakes** (rear wheels on older/economy cars) - Brake shoes push outward against drum

**Advanced Braking Features:**
- **ABS (Anti-lock Braking System)** - Prevents wheel lockup during hard braking
- **EBD (Electronic Brakeforce Distribution)** - Optimizes brake force between wheels
- **Brake Assist** - Detects emergency braking and applies maximum force
- **Electronic Stability Control** - Uses individual wheel braking to maintain control

Regular brake maintenance is essential for safety - brake pads typically need replacement every 30,000-70,000 miles depending on driving conditions!)";
        }
        else if (lowerPrompt.contains("wheel") || lowerPrompt.contains("tire")) {
            return R"(Wheels and tires are your car's crucial connection to the road, affecting safety, handling, and fuel efficiency:

**Wheel Components:**
- **Rim** - The metal outer edge that holds the tire
- **Hub** - Center part that mounts to the vehicle
- **Spokes/Barrel** - Connect rim to hub (steel wheels) or solid design (alloy wheels)
- **Valve Stem** - Where you add air to the tire

**Tire Construction:**
- **Tread** - Outer rubber with patterns for traction
- **Sidewall** - Flexible side portion with tire information
- **Steel Belts** - Reinforcement for strength and stability
- **Bead** - Inner edge that seals against the rim
- **Inner Liner** - Holds air pressure (tubeless tires)

**Tire Types:**
- **All-Season** - Balanced performance for most conditions
- **Summer** - Maximum dry/wet grip, poor in snow
- **Winter** - Specialized rubber compound and tread for snow/ice
- **Performance** - Enhanced handling for sports cars

**Important Tire Specifications:**
- **Size** (e.g., 225/65R17) - Width/Aspect Ratio/Construction/Diameter
- **Load Index** - Maximum weight capacity
- **Speed Rating** - Maximum safe speed
- **DOT Date Code** - Manufacturing date (tires age even unused)

Proper wheel and tire maintenance dramatically improves safety, fuel economy, and tire lifespan!)";
        }
        else if (lowerPrompt.contains("car") || lowerPrompt.contains("auto")) {
            return QString(R"(That's a great automotive question! You asked about: "%1"

Cars are complex machines with thousands of interconnected parts working together. Modern vehicles integrate:

**Major Systems:**
- **Powertrain** (engine, transmission, drivetrain) - Generates and delivers power
- **Chassis** (suspension, steering, brakes) - Handles vehicle control and safety
- **Body/Electrical** (lights, computers, comfort systems) - Protection and functionality
- **HVAC** (heating, ventilation, air conditioning) - Climate control

**Modern Car Technology:**
- **ECUs** (Electronic Control Units) - Up to 100+ computers managing various systems
- **CAN Bus** - Network allowing systems to communicate
- **OBD-II** - Standardized diagnostics port for trouble codes
- **Advanced Driver Assistance** - Lane keeping, adaptive cruise, automatic emergency braking

The automotive industry continues evolving with electric vehicles, autonomous driving technology, and connected car features. Each system requires specific expertise for proper maintenance and repair.

What specific aspect of automotive technology would you like to explore further?)").arg(userPrompt);
        }
        else {
            return QString("I received your question: \"%1\"\n\nI'm a GPT4All-enhanced AI assistant with specialized knowledge in automotive systems and technology. I can provide detailed technical explanations about cars, engines, brakes, electrical systems, and more.\n\nThis server demonstrates how GPT4All's HTTP infrastructure can be enhanced with domain-specific knowledge. What would you like to know about cars or technology?").arg(userPrompt);
        }
    }
    
    QByteArray createNotFoundResponse() {
        QJsonObject error;
        error["error"] = "Not Found";
        error["message"] = "The requested endpoint was not found";
        return createHttpResponse(404, QJsonDocument(error).toJson(QJsonDocument::Compact));
    }
    
    QByteArray createHttpResponse(int statusCode, const QByteArray &body) {
        QString statusText;
        switch (statusCode) {
            case 200: statusText = "OK"; break;
            case 400: statusText = "Bad Request"; break;
            case 404: statusText = "Not Found"; break;
            default: statusText = "Unknown"; break;
        }
        
        QByteArray response;
        response += QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8();
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
        response += "Access-Control-Allow-Headers: Content-Type\r\n";
        response += QString("Content-Length: %1\r\n").arg(body.length()).toUtf8();
        response += "\r\n";
        response += body;
        
        return response;
    }

private:
    QTcpServer *m_server;
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    qDebug() << "🚀 Starting GPT4All-Enhanced HTTP Server...";
    
    SimpleHTTPServer server;
    
    if (server.startServer(4891)) {
        qDebug() << "✅ Server started successfully!";
        qDebug() << "   Press Ctrl+C to stop the server.";
        return app.exec();
    } else {
        qDebug() << "❌ Failed to start server";
        return 1;
    }
}

#include "standalone_enhanced_server.moc"
