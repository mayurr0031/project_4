from flask import Flask, render_template, jsonify, request
from flask_cors import CORS
import mysql.connector
from mysql.connector import Error
from datetime import datetime, timedelta
import json

app = Flask(__name__)
CORS(app)

# ===================== CONFIGURATION =====================
DB_CONFIG = {
    'host': 'localhost',
    'user': 'root',
    'password': 'Govind#15',
    'database': 'energy_meter',
    'auth_plugin': 'mysql_native_password'
}

# ===================== GLOBAL STATE =====================
relay_states = {
    'relay1': False,
    'relay2': False,
    'relay3': True,  # Main relay (theft protection)
    'last_updated': None
}

theft_status = {
    'detected': False,
    'timestamp': None
}

price_per_unit = 5.0  # Default price per kWh

# ===================== DATABASE FUNCTIONS =====================
def get_db_connection():
    """Create and return database connection"""
    try:
        connection = mysql.connector.connect(**DB_CONFIG)
        return connection
    except Error as e:
        print(f"❌ Error connecting to MySQL: {e}")
        return None

def init_database():
    """Initialize database and create tables"""
    try:
        connection = mysql.connector.connect(
            host=DB_CONFIG['host'],
            user=DB_CONFIG['user'],
            password=DB_CONFIG['password'],
            auth_plugin=DB_CONFIG['auth_plugin']
        )
        cursor = connection.cursor()
        
        cursor.execute("CREATE DATABASE IF NOT EXISTS energy_meter")
        cursor.execute("USE energy_meter")
        
        # Updated readings table with energy fields
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS readings (
                id INT AUTO_INCREMENT PRIMARY KEY,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                voltage FLOAT,
                current1 FLOAT,
                current2 FLOAT,
                current3 FLOAT,
                total_current FLOAT,
                power1 FLOAT,
                power2 FLOAT,
                total_power FLOAT,
                energy_l1 FLOAT DEFAULT 0,
                energy_l2 FLOAT DEFAULT 0,
                total_energy FLOAT DEFAULT 0,
                cost_l1 FLOAT DEFAULT 0,
                cost_l2 FLOAT DEFAULT 0,
                total_cost FLOAT DEFAULT 0,
                theft_detected BOOLEAN DEFAULT FALSE,
                relay1_state BOOLEAN,
                relay2_state BOOLEAN,
                INDEX idx_timestamp (timestamp)
            )
        """)
        
        # Settings table for price
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS settings (
                id INT PRIMARY KEY DEFAULT 1,
                price_per_unit FLOAT DEFAULT 5.0,
                last_updated DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
            )
        """)
        
        # Insert default settings if not exists
        cursor.execute("""
            INSERT IGNORE INTO settings (id, price_per_unit) VALUES (1, 5.0)
        """)
        
        connection.commit()
        cursor.close()
        connection.close()
        print("✅ Database initialized successfully")
    except Error as e:
        print(f"❌ Error initializing database: {e}")

# ===================== API ENDPOINTS =====================

@app.route('/')
def index():
    """Serve main dashboard page"""
    return render_template('index.html')

@app.route('/api/data', methods=['POST'])
def receive_data():
    """Receive complete sensor data from ESP32"""
    try:
        data = request.get_json()
        print(f"📊 Data received: Power={data.get('total_power', 0):.2f}W, "
              f"Energy={data.get('total_energy', 0):.3f}kWh, "
              f"Theft={data.get('theft_detected', False)}")
        
        connection = get_db_connection()
        if not connection:
            return jsonify({'status': 'error', 'message': 'Database connection failed'}), 500
        
        cursor = connection.cursor()
        
        # Update theft status
        if data.get('theft_detected', False):
            theft_status['detected'] = True
            theft_status['timestamp'] = datetime.now().isoformat()
        
        query = """
            INSERT INTO readings 
            (voltage, current1, current2, current3, total_current, 
             power1, power2, total_power, energy_l1, energy_l2, total_energy,
             cost_l1, cost_l2, total_cost, theft_detected, relay1_state, relay2_state)
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        """
        
        values = (
            float(data.get('voltage', 0)),
            float(data.get('current1', 0)),
            float(data.get('current2', 0)),
            float(data.get('current3', 0)),
            float(data.get('total_current', 0)),
            float(data.get('power1', 0)),
            float(data.get('power2', 0)),
            float(data.get('total_power', 0)),
            float(data.get('energy_l1', 0)),
            float(data.get('energy_l2', 0)),
            float(data.get('total_energy', 0)),
            float(data.get('cost_l1', 0)),
            float(data.get('cost_l2', 0)),
            float(data.get('total_cost', 0)),
            bool(data.get('theft_detected', False)),
            relay_states['relay1'],
            relay_states['relay2']
        )
        
        cursor.execute(query, values)
        connection.commit()
        
        cursor.close()
        connection.close()
        
        return jsonify({'status': 'success', 'message': 'Data saved'}), 200
        
    except Exception as e:
        print(f"❌ Error saving data: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/latest', methods=['GET'])
def get_latest():
    """Get the most recent reading"""
    try:
        connection = get_db_connection()
        if not connection:
            return jsonify({'status': 'error'}), 500
        
        cursor = connection.cursor(dictionary=True)
        cursor.execute("""
            SELECT * FROM readings 
            ORDER BY timestamp DESC 
            LIMIT 1
        """)
        
        result = cursor.fetchone()
        cursor.close()
        connection.close()
        
        if result:
            result['timestamp'] = result['timestamp'].isoformat()
            result['relay1_state'] = relay_states['relay1']
            result['relay2_state'] = relay_states['relay2']
            result['relay3_state'] = relay_states['relay3']
            result['theft_status'] = theft_status
            return jsonify(result), 200
        else:
            return jsonify({'status': 'no data'}), 404
            
    except Exception as e:
        print(f"❌ Error fetching latest data: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/history', methods=['GET'])
def get_history():
    """Get historical data with flexible time range"""
    try:
        # Get parameters
        hours = request.args.get('hours', type=int)
        days = request.args.get('days', type=int)
        start_date = request.args.get('start_date')
        end_date = request.args.get('end_date')
        
        connection = get_db_connection()
        if not connection:
            return jsonify({'status': 'error'}), 500
        
        cursor = connection.cursor(dictionary=True)
        
        # Build query based on parameters
        if start_date and end_date:
            query = """
                SELECT * FROM readings 
                WHERE timestamp BETWEEN %s AND %s
                ORDER BY timestamp ASC
            """
            cursor.execute(query, (start_date, end_date))
        elif days:
            query = """
                SELECT * FROM readings 
                WHERE timestamp >= NOW() - INTERVAL %s DAY
                ORDER BY timestamp ASC
            """
            cursor.execute(query, (days,))
        else:
            # Default to hours (24 if not specified)
            hours = hours or 24
            query = """
                SELECT * FROM readings 
                WHERE timestamp >= NOW() - INTERVAL %s HOUR
                ORDER BY timestamp ASC
            """
            cursor.execute(query, (hours,))
        
        results = cursor.fetchall()
        
        cursor.close()
        connection.close()
        
        for row in results:
            row['timestamp'] = row['timestamp'].isoformat()
        
        print(f"📈 Retrieved {len(results)} historical records")
        return jsonify(results), 200
        
    except Exception as e:
        print(f"❌ Error fetching history: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/stats', methods=['GET'])
def get_stats():
    """Get statistics for specified period"""
    try:
        period = request.args.get('period', 'day')
        
        connection = get_db_connection()
        if not connection:
            return jsonify({'status': 'error'}), 500
        
        cursor = connection.cursor(dictionary=True)
        
        intervals = {'hour': 1/24, 'day': 1, 'week': 7, 'month': 30}
        days = intervals.get(period, 1)
        
        query = """
            SELECT 
                COUNT(*) as total_readings,
                AVG(voltage) as avg_voltage,
                AVG(total_current) as avg_current,
                AVG(total_power) as avg_power,
                MAX(total_power) as max_power,
                MIN(total_power) as min_power,
                MAX(total_energy) as total_energy_kwh,
                MAX(total_cost) as total_cost
            FROM readings
            WHERE timestamp >= NOW() - INTERVAL %s DAY
        """
        
        cursor.execute(query, (days,))
        result = cursor.fetchone()
        
        cursor.close()
        connection.close()
        
        return jsonify(result), 200
        
    except Exception as e:
        print(f"❌ Error fetching stats: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

# ===================== RELAY CONTROL =====================

@app.route('/api/relay/state', methods=['POST'])
def update_relay_state():
    """ESP32 posts relay state updates"""
    global relay_states
    try:
        data = request.get_json()
        
        if 'relay1' in data:
            relay_states['relay1'] = bool(data['relay1'])
        if 'relay2' in data:
            relay_states['relay2'] = bool(data['relay2'])
        if 'relay3' in data:
            relay_states['relay3'] = bool(data['relay3'])
        
        relay_states['last_updated'] = datetime.now().isoformat()
        
        print(f"🔌 Relay states updated: R1={relay_states['relay1']}, "
              f"R2={relay_states['relay2']}, R3={relay_states['relay3']}")
        
        return jsonify({
            'status': 'success',
            'relay1': relay_states['relay1'],
            'relay2': relay_states['relay2'],
            'relay3': relay_states['relay3'],
            'timestamp': relay_states['last_updated']
        }), 200
        
    except Exception as e:
        print(f"❌ Error updating relay state: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/relay/state', methods=['GET'])
def get_relay_state():
    """Get current relay states and settings"""
    try:
        connection = get_db_connection()
        if connection:
            cursor = connection.cursor(dictionary=True)
            cursor.execute("SELECT price_per_unit FROM settings WHERE id = 1")
            result = cursor.fetchone()
            cursor.close()
            connection.close()
            
            if result:
                global price_per_unit
                price_per_unit = result['price_per_unit']
        
        return jsonify({
            'relay1': relay_states['relay1'],
            'relay2': relay_states['relay2'],
            'relay3': relay_states['relay3'],
            'price': price_per_unit,
            'timestamp': relay_states['last_updated']
        }), 200
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/relay/control', methods=['POST'])
def control_relay():
    """Web dashboard sends relay control commands"""
    global relay_states
    try:
        data = request.get_json()
        relay = data.get('relay')
        state = data.get('state')
        
        if relay not in [1, 2, 3]:
            return jsonify({'status': 'error', 'message': 'Invalid relay'}), 400
        
        relay_key = f'relay{relay}'
        relay_states[relay_key] = state
        relay_states['last_updated'] = datetime.now().isoformat()
        
        # If relay3 is turned on, clear theft alert
        if relay == 3 and state:
            theft_status['detected'] = False
            theft_status['timestamp'] = None
            print("✅ Theft alert cleared via web dashboard")
        
        print(f"🌐 Web command: Relay {relay} → {'ON' if state else 'OFF'}")
        
        return jsonify({
            'status': 'success',
            'relay': relay,
            'state': state
        }), 200
        
    except Exception as e:
        print(f"❌ Error controlling relay: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/theft/alert', methods=['POST'])
def theft_alert():
    """Receive theft alert from ESP32"""
    global theft_status
    try:
        data = request.get_json()
        detected = data.get('theft_detected', False)
        
        theft_status['detected'] = detected
        theft_status['timestamp'] = datetime.now().isoformat() if detected else None
        
        print(f"🚨 Theft status updated: {detected}")
        
        return jsonify({'status': 'success'}), 200
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/settings/price', methods=['POST'])
def update_price():
    """Update price per unit"""
    global price_per_unit
    try:
        data = request.get_json()
        new_price = float(data.get('price', 5.0))
        
        connection = get_db_connection()
        if connection:
            cursor = connection.cursor()
            cursor.execute("UPDATE settings SET price_per_unit = %s WHERE id = 1", (new_price,))
            connection.commit()
            cursor.close()
            connection.close()
            
            price_per_unit = new_price
            print(f"💰 Price updated to ₹{new_price} per kWh")
            
            return jsonify({'status': 'success', 'price': new_price}), 200
        else:
            return jsonify({'status': 'error'}), 500
            
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 500

# ===================== MAIN =====================
if __name__ == '__main__':
    print("\n" + "="*60)
    print("⚡ SMART ENERGY METER v2.0 - Flask Server")
    print("="*60 + "\n")
    
    init_database()
    
    print("\n📋 Features Enabled:")
    print("   ✅ Real-time monitoring")
    print("   ✅ Theft detection & alerts")
    print("   ✅ Energy consumption tracking")
    print("   ✅ Cost calculator")
    print("   ✅ Historical data with flexible ranges")
    print("   ✅ Relay control with theft protection")
    
    print("\n🌐 Access Dashboard:")
    print("   • Local:   http://localhost:5000")
    print("   • Network: http://192.168.31.222:5000")
    
    print("\n" + "="*60)
    print("✅ Server starting...\n")
    
    app.run(host='0.0.0.0', port=5000, debug=True)