export default class Server{
    constructor(){
        this.endpoint = '/internal';
    }

    call(key, data){
        return fetch(this.endpoint, {
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json'
            },
            method: 'POST',
            body: JSON.stringify({ key, data })
        })
            .then( res => res.text() )
            .then( res => JSON.parse(res) );
    }
}
