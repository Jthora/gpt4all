#!/usr/bin/env node
/**
 * Node.js integration test for MinimalSimpleServer
 * Tests compatibility with JavaScript/Node.js applications
 */

const http = require('http');
const { URL } = require('url');

class NodeAPITester {
    constructor(baseUrl = 'http://localhost:4891') {
        this.baseUrl = baseUrl;
        this.testsPassed = 0;
        this.testsFailed = 0;
    }

    async makeRequest(path, method = 'GET') {
        return new Promise((resolve, reject) => {
            const url = new URL(path, this.baseUrl);
            const options = {
                hostname: url.hostname,
                port: url.port,
                path: url.pathname,
                method: method,
                headers: {
                    'User-Agent': 'NodeJS-Test-Client/1.0'
                }
            };

            const req = http.request(options, (res) => {
                let data = '';
                res.on('data', chunk => data += chunk);
                res.on('end', () => {
                    try {
                        const json = data ? JSON.parse(data) : null;
                        resolve({
                            statusCode: res.statusCode,
                            headers: res.headers,
                            data: json,
                            raw: data
                        });
                    } catch (e) {
                        resolve({
                            statusCode: res.statusCode,
                            headers: res.headers,
                            data: null,
                            raw: data
                        });
                    }
                });
            });

            req.on('error', reject);
            req.setTimeout(5000, () => {
                req.abort();
                reject(new Error('Request timeout'));
            });
            req.end();
        });
    }

    async runTest(testName, testFunc) {
        process.stdout.write(`Testing ${testName}... `);
        try {
            const result = await testFunc();
            if (result) {
                console.log('✅ PASS');
                this.testsPassed++;
                return true;
            } else {
                console.log('❌ FAIL');
                this.testsFailed++;
                return false;
            }
        } catch (error) {
            console.log(`❌ FAIL - ${error.message}`);
            this.testsFailed++;
            return false;
        }
    }

    async testHealthEndpoint() {
        const response = await this.makeRequest('/health');
        if (response.statusCode !== 200) return false;
        
        const data = response.data;
        const requiredFields = ['status', 'timestamp', 'service'];
        return requiredFields.every(field => field in data) && data.status === 'ok';
    }

    async testModelsEndpoint() {
        const response = await this.makeRequest('/v1/models');
        if (response.statusCode !== 200) return false;
        
        const data = response.data;
        if (data.object !== 'list' || !('data' in data)) return false;
        if (!Array.isArray(data.data) || data.data.length === 0) return false;
        
        const model = data.data[0];
        const requiredFields = ['id', 'object', 'owned_by'];
        return requiredFields.every(field => field in model);
    }

    async testErrorHandling() {
        const response = await this.makeRequest('/nonexistent');
        return response.statusCode === 404;
    }

    async testCorsHeaders() {
        const response = await this.makeRequest('/health');
        const headers = response.headers;
        return 'access-control-allow-origin' in headers && 
               'access-control-allow-methods' in headers;
    }

    async testOptionsMethod() {
        const response = await this.makeRequest('/health', 'OPTIONS');
        return response.statusCode === 200;
    }

    async testConcurrentRequests() {
        const numRequests = 15;
        const promises = Array(numRequests).fill().map(() => this.makeRequest('/health'));
        
        try {
            const responses = await Promise.all(promises);
            return responses.every(response => response.statusCode === 200);
        } catch (error) {
            return false;
        }
    }

    async testResponseTime() {
        const start = Date.now();
        const response = await this.makeRequest('/health');
        const end = Date.now();
        
        const responseTime = end - start;
        return response.statusCode === 200 && responseTime < 100;
    }

    async testContentType() {
        const response = await this.makeRequest('/health');
        return response.headers['content-type'] && 
               response.headers['content-type'].includes('application/json');
    }

    async testMultipleSequentialRequests() {
        for (let i = 0; i < 10; i++) {
            const response = await this.makeRequest('/health');
            if (response.statusCode !== 200) return false;
            await new Promise(resolve => setTimeout(resolve, 50));
        }
        return true;
    }

    async testOpenAICompatibility() {
        const response = await this.makeRequest('/v1/models');
        if (response.statusCode !== 200) return false;
        
        const data = response.data;
        
        // Check OpenAI-compatible structure
        if (data.object !== 'list') return false;
        if (!('data' in data) || !Array.isArray(data.data)) return false;
        
        // Check model object structure
        for (const model of data.data) {
            const requiredFields = ['id', 'object', 'created', 'owned_by'];
            if (!requiredFields.every(field => field in model)) return false;
            if (model.object !== 'model') return false;
        }
        
        return true;
    }

    async waitForServer() {
        console.log('Waiting for server to be ready...');
        const maxWait = 10;
        
        for (let i = 0; i < maxWait; i++) {
            try {
                const response = await this.makeRequest('/health');
                if (response.statusCode === 200) {
                    return true;
                }
            } catch (error) {
                // Server not ready yet
            }
            await new Promise(resolve => setTimeout(resolve, 1000));
        }
        
        throw new Error('Server did not become ready within timeout');
    }

    async runAllTests() {
        console.log('🚀 Starting Node.js Integration Tests for MinimalSimpleServer');
        console.log('='.repeat(60));
        
        try {
            await this.waitForServer();
            console.log('✅ Server is ready!\n');
        } catch (error) {
            console.log('❌ Server not responding, aborting tests');
            return false;
        }
        
        // Run all tests
        await this.runTest('Health endpoint functionality', () => this.testHealthEndpoint());
        await this.runTest('Models endpoint functionality', () => this.testModelsEndpoint());
        await this.runTest('404 error handling', () => this.testErrorHandling());
        await this.runTest('CORS headers present', () => this.testCorsHeaders());
        await this.runTest('OPTIONS method support', () => this.testOptionsMethod());
        await this.runTest('JSON content type', () => this.testContentType());
        await this.runTest('Response time < 100ms', () => this.testResponseTime());
        await this.runTest('Concurrent requests (15)', () => this.testConcurrentRequests());
        await this.runTest('Sequential requests', () => this.testMultipleSequentialRequests());
        await this.runTest('OpenAI API compatibility', () => this.testOpenAICompatibility());
        
        // Results
        console.log('\n' + '='.repeat(60));
        console.log('📊 Test Results:');
        console.log(`✅ Passed: ${this.testsPassed}`);
        console.log(`❌ Failed: ${this.testsFailed}`);
        console.log(`📈 Success Rate: ${(this.testsPassed/(this.testsPassed + this.testsFailed)*100).toFixed(1)}%`);
        
        if (this.testsFailed === 0) {
            console.log('\n🎉 ALL NODE.JS INTEGRATION TESTS PASSED!');
            console.log('✅ MinimalSimpleServer is fully compatible with Node.js applications!');
            return true;
        } else {
            console.log(`\n⚠️  ${this.testsFailed} tests failed. Please review the issues.`);
            return false;
        }
    }
}

if (require.main === module) {
    const tester = new NodeAPITester();
    tester.runAllTests()
        .then(success => process.exit(success ? 0 : 1))
        .catch(error => {
            console.error('Test suite error:', error);
            process.exit(1);
        });
}
